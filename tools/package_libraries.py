"""Assemble tested native artifacts into npm and Maven distributions; no upload."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import subprocess
import zipfile

parser=argparse.ArgumentParser();parser.add_argument('native_root',type=Path);parser.add_argument('output',type=Path)
args=parser.parse_args();root=Path(__file__).resolve().parents[1];version=(root/'VERSION').read_text().strip()
args.output.mkdir(parents=True,exist_ok=False)
targets={}
for metadata in args.native_root.rglob('native.json'):
    source=metadata.parent;info=json.loads(metadata.read_text());assert info['version']==version
    assert info['platform'] not in targets;targets[info['platform']]=info
    node=root/'javascript/prebuilds'/info['node_target'];node.mkdir(parents=True,exist_ok=False)
    shutil.copy2(source/'ylplugin.node',node/'ylplugin.node')
    jni=root/'jvm/natives/src/main/resources/natives'/info['platform'];jni.mkdir(parents=True,exist_ok=False)
    shutil.copy2(source/info['jni'],jni/info['jni'])
    licenses=root/'javascript/licenses'/info['platform'];shutil.copytree(source/'licenses',licenses)
    shutil.copytree(source/'licenses',root/'jvm/natives/src/main/resources/licenses'/info['platform'])
    for wheel in source.glob('*.whl'):shutil.copy2(wheel,args.output/wheel.name)
assert {'linux-x86_64','linux-arm64','windows-x86_64'}.issubset(targets)
assert any(name.startswith('macos-') for name in targets)
for file in ('LICENSING.md','THIRD_PARTY_NOTICES.md'):shutil.copy2(root/file,root/'javascript'/file)
subprocess.run(['npm','pack','--ignore-scripts','--pack-destination',str(args.output.resolve())],cwd=root/'javascript',check=True)
subprocess.run(['gradle','-p',str(root/'jvm'),'publishAllPublicationsToStagingRepository','--no-daemon'],check=True)
repository=root/'jvm/build/maven-repository'
with zipfile.ZipFile(args.output/f'ylplugin-jvm-{version}.zip','w',zipfile.ZIP_DEFLATED) as bundle:
    for path in sorted(repository.rglob('*')):
        if path.is_file() and path.suffix in ('.jar','.pom','.module') and version in path.parts:
            bundle.write(path,path.relative_to(repository).as_posix())
(args.output/'libraries.json').write_text(json.dumps({'version':version,'commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip(),'targets':targets},indent=2)+'\n')
checksums={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in args.output.iterdir() if p.is_file()}
(args.output/'SHA256SUMS').write_text(''.join(f'{digest}  {name}\n' for name,digest in sorted(checksums.items())))
print('Prepared registry distributions for',sorted(targets))
