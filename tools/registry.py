# SPDX-FileCopyrightText: 2026 Cavanshirpro
# SPDX-License-Identifier: LGPL-3.0-only

"""Prepare checksummed CI distributions and submit a signed Maven Central bundle."""
from pathlib import Path
import argparse
import base64
import hashlib
import json
import os
import re
import subprocess
import tempfile
import time
import urllib.parse
import urllib.request
import uuid
import zipfile
from xml.etree import ElementTree

ROOT=Path(__file__).resolve().parents[1]

def gh_json(path):return json.loads(subprocess.check_output(['gh','api',path],text=True))

def prepare(kind):
    repository=os.environ['GITHUB_REPOSITORY'];run_id=os.environ['RELEASE_RUN_ID']
    if not re.fullmatch(r'[0-9]+',run_id):raise ValueError('invalid CI run ID')
    run=gh_json(f'repos/{repository}/actions/runs/{run_id}')
    if run['conclusion']!='success' or run['status']!='completed' or run['head_branch']!='main' or run['path']!='.github/workflows/libraries.yml':raise ValueError('requires a successful main language-libraries run')
    source=ROOT/'registry-input';manifest=json.loads((source/'libraries.json').read_text());version=manifest['version']
    if not re.fullmatch(r'\d+\.\d+\.\d+',version) or manifest['commit']!=run['head_sha']:raise ValueError('artifact provenance mismatch')
    tag=gh_json(f'repos/{repository}/git/ref/tags/v{version}')['object']
    while tag['type']=='tag':tag=gh_json(f"repos/{repository}/git/tags/{tag['sha']}")['object']
    release=gh_json(f'repos/{repository}/releases/tags/v{version}')
    if release['draft'] or tag['sha']!=manifest['commit']:raise ValueError('publish the matching verified GitHub Release first')
    expected={}
    for line in (source/'SHA256SUMS').read_text().splitlines():
        digest,name=line.split('  ',1)
        if Path(name).name!=name or name in expected or not re.fullmatch(r'[0-9a-f]{64}',digest):raise ValueError('invalid checksum entry')
        if hashlib.sha256((source/name).read_bytes()).hexdigest()!=digest:raise ValueError('distribution SHA256 mismatch')
        expected[name]=digest
    if set(expected)!={p.name for p in source.iterdir() if p.is_file() and p.name!='SHA256SUMS'}:raise ValueError('unexpected or missing artifact')
    identities=json.loads((ROOT/'release/registries.json').read_text())
    if kind=='npm':
        asset=source/f'younglionorganization-ylplugin-{version}.tgz'
        import tarfile
        with tarfile.open(asset) as tar:metadata=json.load(tar.extractfile('package/package.json'))
        if metadata['name']!=identities['npm_package'] or metadata['version']!=version:raise ValueError('npm identity mismatch')
        selected=[asset]
    elif kind=='maven':
        asset=source/f'ylplugin-jvm-{version}.zip';found=set();ns={'m':'http://maven.apache.org/POM/4.0.0'}
        with zipfile.ZipFile(asset) as bundle:
            for name in bundle.namelist():
                if name.endswith('.pom'):
                    pom=ElementTree.fromstring(bundle.read(name));group=pom.findtext('m:groupId',namespaces=ns);artifact=pom.findtext('m:artifactId',namespaces=ns)
                    if group!=identities['maven_group'] or pom.findtext('m:version',namespaces=ns)!=version or artifact not in identities['maven_artifacts']:raise ValueError('Maven identity mismatch')
                    if name!=f"{group.replace('.', '/')}/{artifact}/{version}/{artifact}-{version}.pom" or artifact in found:raise ValueError('invalid Maven POM path')
                    prefix=name[:-4]
                    if not all(prefix+suffix in bundle.namelist() for suffix in ('.jar','-sources.jar','-javadoc.jar')):raise ValueError('incomplete Maven binaries/sources/documentation')
                    found.add(artifact)
        if found!=set(identities['maven_artifacts']):raise ValueError('missing Maven publication')
        selected=[asset]
    else:
        from email.parser import BytesParser
        selected=sorted(source.glob('*.whl'))
        if not selected:raise ValueError('no Python wheels')
        for asset in selected:
            if not asset.name.startswith(f'ylplugin-{version}-'):raise ValueError('Python wheel filename identity mismatch')
            with zipfile.ZipFile(asset) as wheel:
                names=[name for name in wheel.namelist() if name.endswith('.dist-info/METADATA')]
                if len(names)!=1:raise ValueError('invalid wheel metadata')
                metadata=BytesParser().parsebytes(wheel.read(names[0]))
                if metadata['Name']!=identities['pypi_project'] or metadata['Version']!=version:raise ValueError('Python identity mismatch')
    destination=ROOT/'registry-ready';destination.mkdir(exist_ok=False)
    import shutil
    for asset in selected:shutil.copy2(asset,destination/asset.name)
    with open(os.environ['GITHUB_ENV'],'a') as env:env.write(f'REGISTRY_VERSION={version}\nREGISTRY_DIRECTORY={destination}\n')
    print('Verified immutable',kind,'distributions for',version)

def central():
    version=os.environ['REGISTRY_VERSION'];source=next(Path(os.environ['REGISTRY_DIRECTORY']).glob('*.zip'))
    username,password=os.environ['MAVEN_CENTRAL_USERNAME'],os.environ['MAVEN_CENTRAL_PASSWORD']
    authorization='Bearer '+base64.b64encode((username+':'+password).encode()).decode()
    with tempfile.TemporaryDirectory(prefix='ylplugin-central-') as temporary:
        root=Path(temporary);home=root/'gnupg';home.mkdir(mode=0o700)
        env=dict(os.environ,GNUPGHOME=str(home))
        subprocess.run(['gpg','--batch','--import'],input=os.environ['MAVEN_GPG_PRIVATE_KEY'].encode(),env=env,capture_output=True,check=True)
        output=subprocess.check_output(['gpg','--batch','--with-colons','--list-secret-keys'],env=env,text=True)
        fingerprint=next(row.split(':')[9] for row in output.splitlines() if row.startswith('fpr:'))
        repo=root/'repository'
        with zipfile.ZipFile(source) as bundle:
            for member in bundle.infolist():
                path=(repo/member.filename).resolve()
                if not path.is_relative_to(repo.resolve()) or member.is_dir() or path.suffix not in ('.pom','.jar','.module'):raise ValueError('invalid Central bundle path')
                path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(bundle.read(member))
        for path in sorted(repo.rglob('*')):
            if path.is_file():subprocess.run(['gpg','--batch','--yes','--pinentry-mode','loopback','--passphrase-fd','0','--local-user',fingerprint,'--armor','--detach-sign',str(path)],input=os.environ['MAVEN_GPG_PASSPHRASE'].encode(),env=env,capture_output=True,check=True)
        for path in sorted(repo.rglob('*')):
            if path.is_file():
                for algorithm in ('md5','sha1','sha256','sha512'):path.with_name(path.name+'.'+algorithm).write_text(hashlib.new(algorithm,path.read_bytes()).hexdigest())
        archive=root/'central.zip'
        with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as bundle:
            for path in sorted(repo.rglob('*')):
                if path.is_file():bundle.write(path,path.relative_to(repo).as_posix())
        boundary='YLPlugin'+uuid.uuid4().hex
        body=(f'--{boundary}\r\nContent-Disposition: form-data; name="bundle"; filename="ylplugin-{version}.zip"\r\nContent-Type: application/zip\r\n\r\n'.encode()+archive.read_bytes()+f'\r\n--{boundary}--\r\n'.encode())
        headers={'Authorization':authorization,'Content-Type':'multipart/form-data; boundary='+boundary}
        query=urllib.parse.urlencode({'name':'YLPlugin '+version,'publishingType':'AUTOMATIC'})
        with urllib.request.urlopen(urllib.request.Request('https://central.sonatype.com/api/v1/publisher/upload?'+query,data=body,headers=headers),timeout=120) as response:deployment=response.read().decode().strip().strip('"')
        print('Central deployment:',deployment)
        for _ in range(120):
            query=urllib.parse.urlencode({'id':deployment})
            with urllib.request.urlopen(urllib.request.Request('https://central.sonatype.com/api/v1/publisher/status?'+query,data=b'',headers={'Authorization':authorization}),timeout=60) as response:status=json.load(response)
            state=status['deploymentState'];print('Central state:',state)
            if state=='PUBLISHED':return
            if state=='FAILED':raise RuntimeError('Central rejected the bundle; inspect deployment '+deployment)
            time.sleep(10)
        raise TimeoutError('Central did not finish; inspect deployment '+deployment+' before retrying')

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('command',choices=['prepare-npm','prepare-pypi','prepare-maven','central']);args=parser.parse_args()
    if args.command=='central':central()
    else:prepare(args.command.removeprefix('prepare-'))
