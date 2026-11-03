# SPDX-FileCopyrightText: 2026 Cavanshirpro
# SPDX-License-Identifier: LGPL-3.0-only

"""Build source/Python-facade release assets from the committed tree (no publication)."""
from pathlib import Path
import hashlib
import json
import os
import re
import subprocess
import sys
import tomllib

root = Path(__file__).resolve().parents[1]
assert not subprocess.check_output(["git", "status", "--porcelain", "--untracked-files=all"], cwd=root), "release inputs must match a clean committed tree"
version = tomllib.loads((root / "pyproject.toml").read_text())["project"]["version"]
assert re.fullmatch(r"\d+\.\d+\.\d+", version), "numeric release version required"
output = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else root / "dist"
output.mkdir(parents=True, exist_ok=True)
assert not any(output.iterdir()), "release output directory must be empty"
commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip()
source = output / f"ylplugin-{version}-source.tar.gz"
subprocess.run(["git", "archive", "--format=tar.gz", f"--prefix=YLPlugin-{version}/",
                f"--output={source}", commit], cwd=root, check=True)
subprocess.run([sys.executable, "-m", "pip", "wheel", "--no-deps", "--wheel-dir", str(output), str(root)], check=True)
sample = output / "demo.ylplugin"
sample.write_bytes((root / "examples/artifacts/demo.ylplugin").read_bytes())
assets = {path.name: hashlib.sha256(path.read_bytes()).hexdigest() for path in sorted(output.iterdir()) if path.is_file()}
manifest = {"version": version, "commit": commit, "kind": "experimental-source-and-python-facade", "sha256": assets}
run_id, repository = os.environ.get("GITHUB_RUN_ID"), os.environ.get("GITHUB_REPOSITORY")
if run_id and repository:
    manifest["workflow_run"] = f"https://github.com/{repository}/actions/runs/{run_id}"
(output / "release-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
assets["release-manifest.json"] = hashlib.sha256((output / "release-manifest.json").read_bytes()).hexdigest()
(output / "SHA256SUMS").write_text("".join(f"{digest}  {name}\n" for name, digest in sorted(assets.items())))
print(f"Prepared {len(assets) + 1} assets for v{version} at commit {commit}")
