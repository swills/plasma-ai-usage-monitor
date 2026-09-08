#!/usr/bin/env python3
"""Reject release-media evidence captured from the wrong active window."""

from __future__ import annotations

from pathlib import Path

from release_media_evidence import EvidenceError, validate_capture_evidence


ROOT = Path(__file__).resolve().parents[1]
asset = "overview-popup.png"
valid = {
    asset: {
        "pid": 4242,
        "windowId": "{correct-window}",
        "windowIdentity": '"caption" s "AI Usage Monitor" "pid" u 4242',
        "marker": "Overview view ready",
        "markerBefore": "pid=4242 marker='Overview view ready'",
        "markerAfter": "pid=4242 marker='Overview view ready'",
    }
}
validate_capture_evidence(valid, (asset,))

wrong_window = {asset: dict(valid[asset])}
wrong_window[asset]["windowIdentity"] = (
    '"caption" s "Password Manager" "pid" u 7331'
)
try:
    validate_capture_evidence(wrong_window, (asset,))
except EvidenceError:
    pass
else:
    raise SystemExit("Release-media evidence accepted the wrong active window")

wrong_marker = {asset: dict(valid[asset])}
wrong_marker[asset]["markerAfter"] = "Analyst view ready"
try:
    validate_capture_evidence(wrong_marker, (asset,))
except EvidenceError:
    pass
else:
    raise SystemExit("Release-media evidence accepted a changed AT-SPI view")

wrong_accessible_pid = {asset: dict(valid[asset])}
wrong_accessible_pid[asset]["markerAfter"] = (
    "pid=7331 marker='Overview view ready'"
)
try:
    validate_capture_evidence(wrong_accessible_pid, (asset,))
except EvidenceError:
    pass
else:
    raise SystemExit("Release-media evidence accepted the wrong AT-SPI process")

capture_script = (ROOT / "scripts/demo/capture_v18_media.sh").read_text(
    encoding="utf-8"
)
capture_contract = {
    "per-view isolated HOME": 'HOME="$view_home"',
    "panel isolated HOME": 'HOME="$panel_home"',
    "empty desktop": "desktopsList[d].widgets()",
    "controlled wallpaper": 'wallpaperPlugin = \\"org.kde.image\\"',
    "PID-bound accessibility": '--pid "$WINDOW_PID" --window "$match_query"',
    "manifest evidence": "captureEvidence: $captureEvidence",
    "virtual parent compositor": "kwin_wayland --virtual",
    "input method disabled": "QT_IM_MODULE=",
}
missing = [
    label for label, token in capture_contract.items() if token not in capture_script
]
if missing:
    raise SystemExit(
        "Release-media capture contract is incomplete: " + ", ".join(missing)
    )

print(
    "Release media evidence tests OK: "
    "isolated capture enforced; wrong window, marker, and AT-SPI process rejected"
)

# Development mode must still reject damaged assets; only current-tree binding
# is deferred to candidate qualification.
import json
import shutil
import subprocess
import sys
import tempfile

with tempfile.TemporaryDirectory(prefix="media-evidence-") as directory:
    screenshots = Path(directory) / "screenshots"
    shutil.copytree(ROOT / "assets/screenshots", screenshots)
    manifest_path = screenshots / "v18-media-manifest.json"
    manifest = json.loads(manifest_path.read_text())
    manifest["assets"][asset] = "0" * 64
    manifest_path.write_text(json.dumps(manifest))
    result = subprocess.run(
        [sys.executable, str(ROOT / "scripts/check_release_media.py"),
         "--mode", "development", "--screenshots", str(screenshots)],
        capture_output=True, text=True,
    )
    if result.returncode == 0 or "manifest hash does not match" not in result.stderr:
        raise SystemExit("Development media validation failed to reject damaged asset evidence")
print("Development media integrity regression PASS")
