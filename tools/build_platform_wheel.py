# SPDX-FileCopyrightText: 2026 Cavanshirpro
# SPDX-License-Identifier: LGPL-3.0-only

"""Build and install-test the collected platform's wheel; Linux is auditwheel-verified."""
from pathlib import Path
import json
import platform
import subprocess
import sys

root=Path(__file__).resolve().parents[1]
metadata=list((root/'native-dist').glob('*/native.json'));assert len(metadata)==1
directory=metadata[0].parent;info=json.loads(metadata[0].read_text());wheel=next((root/'build/python-base').glob('*.whl'))
if platform.system()=='Linux':tag='linux_'+('aarch64' if info['platform'].endswith('arm64') else 'x86_64')
elif platform.system()=='Darwin':tag='macosx_11_0_'+('arm64' if info['platform'].endswith('arm64') else 'x86_64')
else:tag='win_amd64'
temporary=root/'build/platform-wheel'
subprocess.run([sys.executable,str(root/'tools/platform_wheel.py'),str(wheel),str(directory/info['runtime']),str(temporary),'--cli',str(directory/info['cli']),'--platform',tag,'--licenses',str(directory/'licenses')],check=True)
platform_wheel=next(temporary.glob('*.whl'))
if platform.system()=='Linux':
    subprocess.run([sys.executable,'-m','pip','install','auditwheel'],check=True)
    arch='aarch64' if info['platform'].endswith('arm64') else 'x86_64'
    subprocess.run(['auditwheel','repair','--plat','manylinux_2_39_'+arch,'--wheel-dir',str(directory),str(platform_wheel)],check=True)
else:
    import shutil
    shutil.copy2(platform_wheel,directory/platform_wheel.name)
ready=next(directory.glob('*.whl'));installed=root/'build/wheel-venv'
subprocess.run([sys.executable,'-m','venv',str(installed)],check=True)
bin_dir=installed/('Scripts' if platform.system()=='Windows' else 'bin')
python=bin_dir/('python.exe' if platform.system()=='Windows' else 'python')
cli=bin_dir/('ylplugin.exe' if platform.system()=='Windows' else 'ylplugin')
subprocess.run([str(python),'-m','pip','install','--no-deps','--force-reinstall',str(ready)],check=True)
fixture=root/'examples/artifacts/data.ylplugin'
snippet="from ylplugin import Library; import sys; sdk=Library(); assert 'xyz.example' in sdk.read_metadata(sys.argv[1]); p=sdk.open(sys.argv[1]); assert b'42' in p.read('catalog.json'); p.validate(); p.close()"
env=dict(__import__('os').environ)
for name in ('PYTHONPATH','YLPLUGIN_LIBRARY','YLPLUGIN_CLI'):env.pop(name,None)
subprocess.run([str(python),'-c',snippet,str(fixture)],env=env,cwd=root/'build',check=True)
assert subprocess.check_output([str(cli),'--version'],env=env,text=True,encoding='utf-8').strip()==info['version']
subprocess.run([str(cli),'validate',str(fixture)],env=env,cwd=root/'build',check=True)
metadata=subprocess.check_output([str(cli),'metadata',str(fixture),'--json'],env=env,text=True,encoding='utf-8')
assert json.loads(metadata)['scope']=='metadata-only'
subprocess.run([str(python),'-m','ylplugin','--version'],env=env,cwd=root/'build',check=True)
print('Installed wheel core, console entry point and module CLI work without SDK environment variables')
