# SPDX-FileCopyrightText: 2026 Cavanshirpro
# SPDX-License-Identifier: LGPL-3.0-only

"""Assemble tested native artifacts into npm and Maven distributions; no upload."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import subprocess
import zipfile
import tarfile
import tempfile
import os
from email.parser import BytesParser

parser=argparse.ArgumentParser();parser.add_argument('native_root',type=Path);parser.add_argument('output',type=Path)
args=parser.parse_args();root=Path(__file__).resolve().parents[1];version=(root/'VERSION').read_text().strip()
args.output.mkdir(parents=True,exist_ok=False)
targets={}
for metadata in args.native_root.rglob('native.json'):
    source=metadata.parent;info=json.loads(metadata.read_text());assert info['version']==version
    assert info['platform'] not in targets;targets[info['platform']]=info
    node=root/'javascript/prebuilds'/info['node_target'];node.mkdir(parents=True,exist_ok=False)
    shutil.copy2(source/'ylplugin.node',node/'ylplugin.node')
    shutil.copy2(source/info['cli'],node/info['cli'])
    if info['platform'].startswith(('linux-','macos-')):(node/info['cli']).chmod(0o755)
    jni=root/'jvm/natives/src/main/resources/natives'/info['platform'];jni.mkdir(parents=True,exist_ok=False)
    shutil.copy2(source/info['jni'],jni/info['jni'])
    shutil.copy2(source/info['cli'],jni/info['cli'])
    licenses=root/'javascript/licenses'/info['platform'];shutil.copytree(source/'licenses',licenses)
    shutil.copytree(source/'licenses',root/'jvm/natives/src/main/resources/licenses'/info['platform'])
    for wheel in source.glob('*.whl'):shutil.copy2(wheel,args.output/wheel.name)
assert {'linux-x86_64','linux-arm64','windows-x86_64'}.issubset(targets)
assert any(name.startswith('macos-') for name in targets)
for file in ('LICENSE','COPYING','LICENSING.md','SOURCES.md','THIRD_PARTY_NOTICES.md'):shutil.copy2(root/file,root/'javascript'/file)
subprocess.run(['npm','pack','--ignore-scripts','--pack-destination',str(args.output.resolve())],cwd=root/'javascript',check=True)
# Exercise the actual npm archive, its installed bin link and bundled core.
with tempfile.TemporaryDirectory(prefix='ylplugin-npm-install-') as temporary:
    installed=Path(temporary)
    subprocess.run(['npm','install','--ignore-scripts','--no-audit','--no-fund',str((args.output/f'younglionorganization-ylplugin-{version}.tgz').resolve())],cwd=installed,check=True)
    env=dict(os.environ)
    for name in ('YLPLUGIN_CLI','YLPLUGIN_NODE_PATH'):env.pop(name,None)
    subprocess.run(['npm','exec','--','ylplugin','validate',str(root/'examples/artifacts/data.ylplugin')],cwd=installed,env=env,check=True)
    script="const sdk=require('@younglionorganization/ylplugin');if(!sdk.readMetadata(process.argv[1]).includes('xyz.example'))process.exit(1)"
    subprocess.run(['node','-e',script,str(root/'examples/artifacts/data.ylplugin')],cwd=installed,env=env,check=True)
subprocess.run(['gradle','-p',str(root/'jvm'),'publishAllPublicationsToStagingRepository',':java:smokeTest',':kotlin:smokeTest','--no-daemon'],check=True)
repository=root/'jvm/build/maven-repository'
with zipfile.ZipFile(args.output/f'ylplugin-jvm-{version}.zip','w',zipfile.ZIP_DEFLATED) as bundle:
    for path in sorted(repository.rglob('*')):
        if path.is_file() and path.suffix in ('.jar','.pom','.module') and version in path.parts:
            bundle.write(path,path.relative_to(repository).as_posix())
(args.output/'libraries.json').write_text(json.dumps({'version':version,'commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip(),'targets':targets},indent=2)+'\n')
checksums={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in args.output.iterdir() if p.is_file()}
# Check the actual packaged license/source payload, not only source metadata.
with tarfile.open(args.output/f'younglionorganization-ylplugin-{version}.tgz') as bundle:
    names=bundle.getnames()
    assert {'package/LICENSE','package/COPYING','package/SOURCES.md'}.issubset(names)
    assert any(name.endswith(f'/YLPlugin-source-{version}.tar.gz') for name in names)
    assert json.load(bundle.extractfile('package/package.json'))['license']=='LGPL-3.0-only'
for path in args.output.glob('*.whl'):
    with zipfile.ZipFile(path) as bundle:
        names=bundle.namelist()
        metadata=next(name for name in names if name.endswith('.dist-info/METADATA'))
        assert BytesParser().parsebytes(bundle.read(metadata))['License-Expression']=='LGPL-3.0-only'
        assert any(name.startswith('ylplugin/_native/ylplugin') for name in names)
        assert b'ylplugin.__main__:main' in bundle.read(next(name for name in names if name.endswith('.dist-info/entry_points.txt')))
        assert any(name.endswith('/licenses/LICENSE') for name in names)
        assert any(name.endswith('/licenses/COPYING') for name in names)
        assert any(name.endswith(f'/YLPlugin-source-{version}.tar.gz') for name in names)
with zipfile.ZipFile(args.output/f'ylplugin-jvm-{version}.zip') as bundle:
    import io
    for name in bundle.namelist():
        if name.endswith('.pom'):
            assert b'GNU Lesser General Public License, version 3.0 only' in bundle.read(name)
        if name.endswith('.jar'):
            with zipfile.ZipFile(io.BytesIO(bundle.read(name))) as jar:
                assert {'META-INF/LICENSE','META-INF/COPYING','META-INF/SOURCES.md'}.issubset(jar.namelist())
                assert any(item.endswith(f'/YLPlugin-source-{version}.tar.gz') for item in jar.namelist())
(args.output/'SHA256SUMS').write_text(''.join(f'{digest}  {name}\n' for name,digest in sorted(checksums.items())))
print('Prepared registry distributions for',sorted(targets))
