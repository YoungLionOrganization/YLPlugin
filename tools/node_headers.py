"""Download the installed Node version's official headers/import library for CMake."""
import json
from pathlib import Path
import re
import subprocess
import tarfile
import urllib.request
import hashlib

root=Path("build/node").resolve();root.mkdir(parents=True,exist_ok=True)
version=subprocess.check_output(["node","--version"],text=True).strip()
assert re.fullmatch(r"v\d+\.\d+\.\d+",version)
archive=root/"headers.tar.gz"
with urllib.request.urlopen(f"https://nodejs.org/download/release/{version}/SHASUMS256.txt",timeout=60) as response:
    sums={line.split()[1]:line.split()[0] for line in response.read().decode().splitlines() if line.strip()}
with urllib.request.urlopen(f"https://nodejs.org/download/release/{version}/node-{version}-headers.tar.gz",timeout=60) as response:archive.write_bytes(response.read())
assert hashlib.sha256(archive.read_bytes()).hexdigest()==sums[f'node-{version}-headers.tar.gz']
with tarfile.open(archive) as bundle:bundle.extractall(root,filter="data")
with urllib.request.urlopen(f"https://raw.githubusercontent.com/nodejs/node/{version}/LICENSE",timeout=60) as response:
    (root/f"node-{version}/LICENSE").write_bytes(response.read())
config={"include":str(root/f"node-{version}/include/node")}
if subprocess.check_output(["node","-p","process.platform"],text=True).strip()=="win32":
    arch=subprocess.check_output(["node","-p","process.arch"],text=True).strip();assert arch in ("x64","arm64")
    library=root/"node.lib"
    with urllib.request.urlopen(f"https://nodejs.org/download/release/{version}/win-{arch}/node.lib",timeout=60) as response:library.write_bytes(response.read())
    assert hashlib.sha256(library.read_bytes()).hexdigest()==sums[f'win-{arch}/node.lib']
    config["library"]=str(library)
(root/"config.json").write_text(json.dumps(config))
print("Node headers prepared for",version)
