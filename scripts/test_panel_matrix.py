#!/usr/bin/env python3
"""Reject stale, incomplete, damaged or wrongly attributed panel evidence."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
identity = subprocess.check_output([sys.executable, str(ROOT / "scripts/demo/source_identity.py")], text=True).strip()
name = "media-panel-dark-1-horizontal-24"

def png(width, height):
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 0, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress((b"\0" * (width + 1)) * height)) + chunk(b"IEND", b""))

with tempfile.TemporaryDirectory(prefix="panel-matrix-contract-") as temporary:
    directory = Path(temporary)
    evidence = dict(scenario="media-panel", scale=1, theme="dark", requestedThickness=24,
                    orientation="horizontal", panelGeometry=[dict(location="bottom", height=24)],
                    plasmashellPid=4242, markerBefore="pid=4242 marker='AI Usage Monitor: 28%'",
                    markerAfter="pid=4242 marker='AI Usage Monitor: 28%'", image=name + ".png")
    manifest = dict(sourceTreeSha256=identity, cases=1, scenarios=["media-panel"], scales=[1],
                    themes=["dark"], sizes=[24], orientations=["horizontal"])
    def write(image):
        (directory / (name + ".png")).write_bytes(image)
        evidence["sha256"] = hashlib.sha256(image).hexdigest()
        payload = json.dumps(evidence).encode()
        (directory / (name + ".json")).write_bytes(payload)
        manifest["artifacts"] = {name + ".json": hashlib.sha256(payload).hexdigest()}
        (directory / "matrix-manifest.json").write_text(json.dumps(manifest))
    def check(expected=None, subset=True):
        command = [sys.executable, str(ROOT / "scripts/check_panel_matrix.py"), str(directory)]
        if subset:
            command.append("--allow-subset")
        result = subprocess.run(command, capture_output=True, text=True)
        if expected is None:
            assert result.returncode == 0, result.stderr
        else:
            assert result.returncode != 0 and expected in result.stderr, result.stderr
    image = png(1600, 1000)
    write(image)
    check()
    check("Incomplete release matrix", subset=False)
    merge_output = directory / "merged"
    duplicate_merge = subprocess.run([
        sys.executable, str(ROOT / "scripts/merge_panel_matrices.py"),
        "--output", str(merge_output), str(directory), str(directory)],
        capture_output=True, text=True)
    assert duplicate_merge.returncode != 0 and "Overlapping partitions" in duplicate_merge.stderr
    assert not merge_output.exists()

    (directory / (name + ".png")).write_bytes(image + b"changed")
    check("screenshot hash mismatch")
    write(png(800, 500))
    check("geometry/scale mismatch")
    evidence["markerAfter"] = "pid=7331 marker='AI Usage Monitor: 28%'"
    write(image)
    check("accessibility identity mismatch")
    evidence["markerAfter"] = evidence["markerBefore"]
    manifest["sourceTreeSha256"] = "0" * 64
    write(image)
    check("source mismatch")
print("Panel evidence regression PASS: integrity, scale, PID, source and complete coverage")
