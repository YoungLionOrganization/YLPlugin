"""Configure static-core SDK bundles while retaining the public shared SDK default."""
from pathlib import Path
import json
import os
import platform
import subprocess

root=Path(__file__).resolve().parents[1]
node=json.loads((root/'build/node/config.json').read_text())
args=['cmake','-S',str(root),'-B','build-sdk','-DCMAKE_BUILD_TYPE=Release','-DYLPLUGIN_YLSPEC_SOURCE=_deps/YLSpec',
      '-DYLPLUGIN_LIBRARY_TYPE=STATIC','-DYLPLUGIN_BUILD_NODE=ON','-DYLPLUGIN_BUILD_JNI=ON','-DYLPLUGIN_BUILD_PYTHON_RUNTIME=ON',
      '-DOPENSSL_USE_STATIC_LIBS=TRUE','-DNODE_INCLUDE_DIR='+node['include']]
if platform.system()=='Windows':
    base=Path(os.environ['VCPKG_INSTALLATION_ROOT'])
    args+=['-DCMAKE_TOOLCHAIN_FILE='+str(base/'scripts/buildsystems/vcpkg.cmake'),'-DVCPKG_TARGET_TRIPLET=x64-windows-static',
           '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded$<$<CONFIG:Debug>:Debug>','-DNODE_LIBRARY='+node['library']]
else:
    args+=['-G','Ninja']
    if platform.system()=='Darwin':
        prefix=lambda name:subprocess.check_output(['brew','--prefix',name],text=True).strip()
        args+=['-DCMAKE_PREFIX_PATH='+prefix('openssl@3')+';'+prefix('nlohmann-json'),'-DZSTD_LIBRARY='+prefix('zstd')+'/lib/libzstd.a','-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0']
    else:
        triplet=subprocess.check_output(['gcc','-print-multiarch'],text=True).strip()
        args+=['-DZSTD_LIBRARY=/usr/lib/'+triplet+'/libzstd.a', '-DZLIB_LIBRARY=/usr/lib/'+triplet+'/libz.so','-DZLIB_LIBRARY_RELEASE=/usr/lib/'+triplet+'/libz.so']
subprocess.run(args,cwd=root,check=True)
