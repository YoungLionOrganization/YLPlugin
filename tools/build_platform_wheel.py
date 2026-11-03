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
subprocess.run([sys.executable,str(root/'tools/platform_wheel.py'),str(wheel),str(directory/info['runtime']),str(temporary),'--platform',tag,'--licenses',str(directory/'licenses')],check=True)
platform_wheel=next(temporary.glob('*.whl'))
if platform.system()=='Linux':
    subprocess.run([sys.executable,'-m','pip','install','auditwheel'],check=True)
    arch='aarch64' if info['platform'].endswith('arm64') else 'x86_64'
    subprocess.run(['auditwheel','repair','--plat','manylinux_2_39_'+arch,'--wheel-dir',str(directory),str(platform_wheel)],check=True)
else:
    import shutil
    shutil.copy2(platform_wheel,directory/platform_wheel.name)
ready=next(directory.glob('*.whl'));installed=root/'build/wheel-installed'
subprocess.run([sys.executable,'-m','pip','install','--no-deps','--target',str(installed),str(ready)],check=True)
snippet="from ylplugin import Library; p=Library().open(r'"+str(root/'examples/artifacts/data.ylplugin')+"'); assert b'42' in p.read('catalog.json'); p.validate(); p.close(); print('Installed platform wheel works without SDK environment variables')"
env=dict(__import__('os').environ,PYTHONPATH=str(installed));env.pop('YLPLUGIN_LIBRARY',None)
subprocess.run([sys.executable,'-c',snippet],env=env,cwd=root/'build',check=True)
