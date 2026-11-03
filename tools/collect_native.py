"""Collect only the actual platform's binaries and required third-party notices."""
from pathlib import Path
import argparse
import json
import os
import platform
import shutil

parser=argparse.ArgumentParser();parser.add_argument('--build',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--ylspec',type=Path,required=True);parser.add_argument('--system-prefix',type=Path,default=Path('/'))
args=parser.parse_args();root=Path(__file__).resolve().parents[1]
os_name={'Linux':'linux','Darwin':'macos','Windows':'windows'}[platform.system()]
arch={'aarch64':'arm64','ARM64':'arm64','AMD64':'x86_64'}.get(platform.machine(),platform.machine())
node_os={'macos':'darwin','windows':'win32'}.get(os_name,os_name);node_arch='x64' if arch=='x86_64' else arch
target=args.output/f'{os_name}-{arch}';target.mkdir(parents=True,exist_ok=False)
suffix='.dll' if os_name=='windows' else '.dylib' if os_name=='macos' else '.so';prefix='' if os_name=='windows' else 'lib'
for filename in ('ylplugin.node',prefix+'ylplugin_jni'+suffix,prefix+'ylplugin_runtime'+suffix):
    candidates=list(args.build.rglob(filename));assert len(candidates)==1,(filename,candidates)
    shutil.copy2(candidates[0],target/filename)
licenses=target/'licenses';licenses.mkdir()
for name,path in {'YLSpec-LICENSE':args.ylspec/'LICENSE','YLSpec-NOTICE':args.ylspec/'NOTICE','YLSpec-third-parties.md':args.ylspec/'THIRD_PARTY_NOTICES.md','tomlc17-LICENSE':args.ylspec/'vendor/tomlc17/LICENSE','YLPlugin-LICENSING.md':root/'LICENSING.md','YLPlugin-THIRD_PARTY_NOTICES.md':root/'THIRD_PARTY_NOTICES.md'}.items():shutil.copy2(path,licenses/name)
if os_name=='windows':
    base=Path(os.environ['VCPKG_INSTALLATION_ROOT'])/'installed/x64-windows-static/share'
    paths={'OpenSSL':base/'openssl/copyright','Zstandard':base/'zstd/copyright','nlohmann-json':base/'nlohmann-json/copyright'}
elif os_name=='linux':
    paths={}
    for name,package in [('OpenSSL','libssl-dev'),('Zstandard','libzstd-dev'),('nlohmann-json','nlohmann-json3-dev')]:
        candidate=args.system_prefix/'usr/share/doc'/package/'copyright'
        paths[name]=candidate if candidate.is_file() else Path('/usr/share/doc')/package/'copyright'
    zlib=args.system_prefix/'usr/share/doc/zlib1g-dev/copyright'
    if zlib.is_file():paths['Zlib']=zlib
else:
    import subprocess
    paths={}
    for name,formula in [('OpenSSL','openssl@3'),('Zstandard','zstd'),('nlohmann-json','nlohmann-json')]:
        base=Path(subprocess.check_output(['brew','--prefix',formula],text=True).strip());candidates=[p for p in base.rglob('*') if p.is_file() and p.name.upper() in ('LICENSE','LICENSE.TXT','LICENSE.MIT','COPYING','COPYRIGHT')]
        assert candidates,(formula,'license missing');paths[name]=candidates[0]
for name,path in paths.items():assert path.is_file(),str(path);shutil.copy2(path,licenses/(name+'-LICENSE'))
node_config=Path('build/node/config.json')
if node_config.is_file():
    node_root=Path(json.loads(node_config.read_text())['include']).parent.parent
    assert (node_root/'LICENSE').is_file(),'Node header license missing'
    shutil.copy2(node_root/'LICENSE',licenses/'Node-LICENSE')
java_home=Path(os.environ['JAVA_HOME'])
java_notices=[p for p in (java_home/'LICENSE',java_home/'legal/java.base/LICENSE',java_home/'legal/java.base/ASSEMBLY_EXCEPTION',java_home/'legal/java.base/ADDITIONAL_LICENSE_INFO') if p.is_file()]
if not any(p.name=='LICENSE' for p in java_notices):
    for fallback in (Path('/usr/share/doc/openjdk-21-jre-headless/copyright'),Path('/usr/share/common-licenses/GPL-2')):
        if fallback.is_file():java_notices.append(fallback)
assert java_notices,'JDK JNI license notices missing'
for notice in java_notices:shutil.copy2(notice,licenses/('OpenJDK-'+notice.name))
info={'platform':os_name+'-'+arch,'node_target':node_os+'-'+node_arch,'version':(root/'VERSION').read_text().strip(),'jni':prefix+'ylplugin_jni'+suffix,'runtime':prefix+'ylplugin_runtime'+suffix}
(target/'native.json').write_text(json.dumps(info,indent=2)+'\n')
print(target)
