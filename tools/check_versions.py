# SPDX-FileCopyrightText: 2026 Cavanshirpro
# SPDX-License-Identifier: LGPL-3.0-only

"""Ensure language metadata matches VERSION without altering released tags."""
from pathlib import Path
import json
import re
import tomllib

root=Path(__file__).resolve().parents[1]
version=(root/"VERSION").read_text().strip()
assert re.fullmatch(r"\d+\.\d+\.\d+",version)
assert tomllib.loads((root/"pyproject.toml").read_text())["project"]["version"]==version
assert json.loads((root/"javascript/package.json").read_text())["version"]==version
for path,pattern in [("bindings/python/ylplugin/__init__.py",r'__version__ = "([^"]+)"'),("jvm/java/src/main/java/xyz/younglion/ylplugin/YLPlugin.java",r'VERSION = "([^"]+)"')]:
    assert re.search(pattern,(root/path).read_text()).group(1)==version,path
print("All language versions match",version)
