# SPDX-FileCopyrightText: 2026 Cavanshirpro
# SPDX-License-Identifier: LGPL-3.0-only

"""Add one self-contained ctypes runtime to a platform-tagged facade wheel."""
from pathlib import Path
import argparse
import base64
import csv
import hashlib
import io
import re
import zipfile

parser=argparse.ArgumentParser()
parser.add_argument("wheel",type=Path);parser.add_argument("runtime",type=Path);parser.add_argument("output",type=Path)
parser.add_argument("--platform",required=True);parser.add_argument("--licenses",type=Path,required=True)
args=parser.parse_args();assert re.fullmatch(r"[a-zA-Z0-9_]+",args.platform)
args.output.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(args.wheel) as source:files={name:source.read(name) for name in source.namelist()}
metadata=next(name for name in files if name.endswith(".dist-info/WHEEL"));directory=metadata.rsplit('/',1)[0]
files[metadata]=("Wheel-Version: 1.0\nGenerator: ylplugin-platform-wheel\nRoot-Is-Purelib: false\nTag: py3-none-"+args.platform+"\n").encode()
files["ylplugin/_native/"+args.runtime.name]=args.runtime.read_bytes()
for path in args.licenses.rglob('*'):
    if path.is_file():files[directory+"/licenses/"+path.relative_to(args.licenses).as_posix()]=path.read_bytes()
record=directory+"/RECORD";files.pop(record,None)
stream=io.StringIO(newline='');writer=csv.writer(stream)
for name,data in sorted(files.items()):writer.writerow((name,"sha256="+base64.urlsafe_b64encode(hashlib.sha256(data).digest()).rstrip(b'=').decode(),str(len(data))))
writer.writerow((record,"",""));files[record]=stream.getvalue().encode()
output=args.output/args.wheel.name.replace("-py3-none-any.whl","-py3-none-"+args.platform+".whl")
with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED) as wheel:
    for name,data in sorted(files.items()):wheel.writestr(name,data)
print(output)
