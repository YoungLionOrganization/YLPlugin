# SPDX-FileCopyrightText: 2026 Cavanshirpro
# SPDX-License-Identifier: LGPL-3.0-only

from pathlib import Path
import hashlib
import os
import subprocess
import sys
import tempfile

root=Path(__file__).resolve().parents[1]
cli,host,library=map(Path,sys.argv[1:4])
with tempfile.TemporaryDirectory() as temporary:
    work=Path(temporary).resolve();output=work/'examples'
    subprocess.run([sys.executable,str(root/'examples/build_examples.py'),str(output),'--cli',str(cli),'--native-library',str(library)],check=True)
    approved=hashlib.sha256(library.read_bytes()).hexdigest()
    result=subprocess.run([str(host),str(output/'native.ylplugin'),str(work/'cache'),approved],capture_output=True,text=True)
    assert result.returncode==(4 if os.name=='nt' else 0),result.stdout+result.stderr
    assert not (work/'cache').exists()
    if os.name!='nt':
        rejected=subprocess.run([str(host),str(output/'native.ylplugin'),str(work/'denied-cache'),'00'*32],capture_output=True,text=True)
        assert rejected.returncode!=0 and not (work/'denied-cache').exists()
    for name in ('data','wasm'):
        assert (output/(name+'.ylplugin')).read_bytes()==(root/'examples/artifacts'/(name+'.ylplugin')).read_bytes()
print('Data/WASM reproducibility and digest-approved native example lifecycle passed')
