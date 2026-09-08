#!/usr/bin/env python3
"""Combine independently verified same-source panel matrix partitions."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def merge(inputs, output):
    if output.exists():
        raise ValueError('Output must be new; captured evidence is never overwritten')
    manifests = []
    for directory in inputs:
        subprocess.run([sys.executable, str(ROOT / 'scripts/check_panel_matrix.py'),
                        '--allow-subset', str(directory)], check=True)
        manifests.append(json.loads((directory / 'matrix-manifest.json').read_text()))
    sources = {m['sourceTreeSha256'] for m in manifests}
    if len(sources) != 1:
        raise ValueError('Partitions must identify exactly one source tree')
    names = [name for m in manifests for name in m['artifacts']]
    if len(names) != len(set(names)):
        raise ValueError('Overlapping partitions are not independent coverage')
    output.mkdir(parents=True)
    for directory, manifest in zip(inputs, manifests):
        for name in manifest['artifacts']:
            shutil.copy2(directory / name, output / name)
            png = Path(name).with_suffix('.png')
            shutil.copy2(directory / png, output / png)
    result = dict(sourceTreeSha256=sources.pop(),
                  candidateCommit=manifests[0]['candidateCommit'],
                  cases=sum(m['cases'] for m in manifests),
                  physicalDesktop='unverified', keyboardNavigation='unverified',
                  partitions=[dict(directory=str(directory), manifestSha256=hashlib.sha256(
                      (directory / 'matrix-manifest.json').read_bytes()).hexdigest()) for directory in inputs])
    for dimension in ('scenarios', 'scales', 'themes', 'sizes', 'orientations'):
        result[dimension] = sorted({v for m in manifests for v in m[dimension]})
    result['artifacts'] = {name: digest for m in manifests for name, digest in m['artifacts'].items()}
    (output / 'matrix-manifest.json').write_text(json.dumps(result, indent=2) + '\n')
    subprocess.run([sys.executable, str(ROOT / 'scripts/check_panel_matrix.py'), str(output)], check=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('inputs', type=Path, nargs='+')
    args = parser.parse_args()
    merge(args.inputs, args.output)
