#!/usr/bin/env python3
"""Hash the current capture inputs with the release-media inventory contract."""
import hashlib
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[2]
paths = subprocess.check_output([
    "git", "ls-files", "--cached", "--others", "--exclude-standard", "--",
    "CMakeLists.txt", "package", "plugin", "scripts/demo",
], cwd=root, text=True).splitlines()
inventory = "".join(hashlib.sha256((root / path).read_bytes()).hexdigest()
                    + "  " + path + "\n" for path in sorted(set(paths))
                    if (root / path).is_file())
print(hashlib.sha256(inventory.encode()).hexdigest())
