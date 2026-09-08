#!/usr/bin/env python3
"""Validate panel matrix provenance, coverage and per-case captured evidence."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("directory", type=Path)
parser.add_argument("--allow-subset", action="store_true", help="Validate a diagnostic subset, not full release coverage")
args = parser.parse_args()
manifest = json.loads((args.directory / "matrix-manifest.json").read_text())
current = subprocess.check_output([sys.executable, str(ROOT / "scripts/demo/source_identity.py")], text=True).strip()
if manifest.get("sourceTreeSha256") != current:
    raise SystemExit("Panel matrix source mismatch; recapture the current candidate")
expected = {
    "scenarios": {"media-panel", "media-panel-disconnected", "media-panel-zero", "media-panel-full", "plugin-recovery"},
    "scales": {1, 1.25, 1.5, 2}, "themes": {"light", "dark"},
    "sizes": {24, 32, 48, 64}, "orientations": {"horizontal", "vertical"},
}
for key, values in expected.items():
    recorded = manifest.get(key, [])
    if len(recorded) != len(set(recorded)) or not set(recorded).issubset(values) or not recorded:
        raise SystemExit(f"Invalid matrix dimension: {key}")
    if not args.allow_subset and set(recorded) != values:
        raise SystemExit(f"Incomplete release matrix dimension: {key}")
count = 0
records = set()
for scenario, scale, theme, size, orientation in itertools.product(
    manifest["scenarios"], manifest["scales"], manifest["themes"], manifest["sizes"], manifest["orientations"]
):
    name = f"{scenario}-{theme}-{scale:g}-{orientation}-{size}"
    path = args.directory / f"{name}.json"
    payload = path.read_bytes()
    records.add(path.name)
    if manifest.get("artifacts", {}).get(path.name) != hashlib.sha256(payload).hexdigest():
        raise SystemExit(f"Evidence record hash mismatch: {name}")
    record = json.loads(payload)
    if (record.get("scenario"), record.get("scale"), record.get("theme"), record.get("requestedThickness"), record.get("orientation")) != (scenario, scale, theme, size, orientation):
        raise SystemExit(f"Evidence case identity mismatch: {name}")
    location = "bottom" if orientation == "horizontal" else "left"
    if record.get("panelGeometry") != [{"location": location, "height": size}]:
        raise SystemExit(f"Panel geometry mismatch: {name}")
    pid = record.get("plasmashellPid")
    marker = {"plugin-recovery": "AI Usage Monitor needs its native plugin", "media-panel-zero": "AI Usage Monitor: 0%", "media-panel-full": "AI Usage Monitor: 100%", "media-panel": "AI Usage Monitor: 28%"}.get(scenario, "AI Usage Monitor:")
    for key in ("markerBefore", "markerAfter"):
        if not isinstance(pid, int) or pid <= 0 or not re.search(rf"\bpid={pid}\b", record.get(key, "")) or marker not in record.get(key, ""):
            raise SystemExit(f"Panel accessibility identity mismatch: {name}/{key}")
    if record.get("image") != f"{name}.png":
        raise SystemExit(f"Invalid image path: {name}")
    image = (args.directory / record["image"]).read_bytes()
    if image[:8] != b"\x89PNG\r\n\x1a\n" or hashlib.sha256(image).hexdigest() != record.get("sha256"):
        raise SystemExit(f"Panel screenshot hash mismatch: {name}")
    if len(image) < 24 or image[12:16] != b"IHDR" or struct.unpack(">II", image[16:24]) != (round(1600 * scale), round(1000 * scale)):
        raise SystemExit(f"Panel screenshot geometry/scale mismatch: {name}")
    count += 1
if manifest.get("cases") != count or set(manifest.get("artifacts", {})) != records:
    raise SystemExit("Panel matrix case count/inventory mismatch")
print(f"Panel matrix {'subset' if args.allow_subset else 'release coverage'} PASS: {count} cases, current source, image hashes and PID-bound markers")
