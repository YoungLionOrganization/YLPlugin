"""Create data, WASM and optionally a platform-specific native container."""
from pathlib import Path
import argparse
import hashlib
import json
import platform
import subprocess

parser=argparse.ArgumentParser()
parser.add_argument("output",type=Path)
parser.add_argument("--cli",type=Path,required=True)
parser.add_argument("--native-library",type=Path)
args=parser.parse_args()
root=Path(__file__).resolve().parent
args.output.mkdir(parents=True,exist_ok=False)

def create(name,manifest,records):
    plan=args.output/(name+"-resources.json")
    plan.write_text(json.dumps({"resources":records}))
    output=args.output/(name+".ylplugin")
    subprocess.run([str(args.cli),"create","--manifest",str(manifest),"--plan",str(plan),"--output",str(output)],check=True)
    subprocess.run([str(args.cli),"validate",str(output)],check=True)

create("data",root/"data/plugin.ylspec",[{"path":"catalog.json","file":str(root/"data/catalog.json"),"media":"application/json"}])
create("wasm",root/"wasm/plugin.ylspec",[{"path":"add.wasm","file":str(root/"wasm/add.wasm"),"media":"application/wasm"}])
if args.native_library:
    library=args.native_library.resolve()
    os_name={"Linux":"linux","Darwin":"macos","Windows":"windows"}[platform.system()]
    arch={"aarch64":"arm64","ARM64":"arm64","AMD64":"x86_64"}.get(platform.machine(),platform.machine())
    resource="plugin"+library.suffix
    manifest=args.output/"native.ylspec"
    manifest.write_text(f'''[_ylspec]
version="0.1"
profile="xyz.younglion.plugin"
profile_version="1.0"
[data]
id="xyz.younglion.examples.native"
version="1.0"
[[data.entrypoints]]
id="native"
runtime="native"
runtime_min="1.0"
runtime_max="1.0"
resource="{resource}"
os="{os_name}"
arch="{arch}"
abi="ylp-c1"
[[data.targets]]
id="native"
host="xyz.example.native-host"
api_min="1.0"
api_max="1.0"
entrypoint="native"
required_permissions=[]
''')
    create("native",manifest,[{"path":resource,"file":str(library),"extraction":2}])
    print("Host approval SHA256:",hashlib.sha256(library.read_bytes()).hexdigest())
