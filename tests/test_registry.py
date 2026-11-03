"""Exercise publication gates offline. No credential or registry endpoint is used."""
import importlib.util
import io
import json
import hashlib
import os
from pathlib import Path
import tarfile
import tempfile
import unittest
from unittest.mock import patch
import zipfile

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('registry',ROOT/'tools/registry.py');registry=importlib.util.module_from_spec(spec);spec.loader.exec_module(registry)

class Gates(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name);registry.ROOT=self.root
        (self.root/'release').mkdir();(self.root/'release/registries.json').write_bytes((ROOT/'release/registries.json').read_bytes())
        self.source=self.root/'registry-input';self.source.mkdir()
        self.sha='a'*40;self.run={'status':'completed','conclusion':'success','head_branch':'main','path':'.github/workflows/libraries.yml','head_sha':self.sha}
        (self.source/'libraries.json').write_text(json.dumps({'version':'0.1.1','commit':self.sha}))
        self.env=patch.dict(os.environ,{'GITHUB_REPOSITORY':'YoungLionOrganization/YLPlugin','RELEASE_RUN_ID':'123','GITHUB_ENV':str(self.root/'environment')});self.env.start()
        def api(path):
            if '/actions/runs/' in path:return self.run
            if '/git/ref/' in path:return {'object':{'type':'commit','sha':self.sha}}
            if '/releases/tags/' in path:return {'draft':False}
            raise AssertionError(path)
        self.api=patch.object(registry,'gh_json',side_effect=api);self.api.start()
    def tearDown(self):self.api.stop();self.env.stop();self.temp.cleanup()
    def sums(self):
        (self.source/'SHA256SUMS').write_text(''.join(hashlib.sha256(path.read_bytes()).hexdigest()+'  '+path.name+'\n' for path in sorted(self.source.iterdir()) if path.name!='SHA256SUMS'))
    def npm(self,name='@younglionorganization/ylplugin'):
        path=self.source/'younglionorganization-ylplugin-0.1.1.tgz'
        data=json.dumps({'name':name,'version':'0.1.1'}).encode()
        with tarfile.open(path,'w:gz') as archive:
            item=tarfile.TarInfo('package/package.json');item.size=len(data);archive.addfile(item,io.BytesIO(data))
        return path
    def test_npm_verified_bytes_staged(self):
        original=self.npm();self.sums();registry.prepare('npm')
        self.assertEqual(original.read_bytes(),(self.root/'registry-ready'/original.name).read_bytes())
    def test_wrong_npm_owner_rejected(self):
        self.npm('@someone/ylplugin');self.sums()
        with self.assertRaisesRegex(ValueError,'identity'):registry.prepare('npm')
    def test_tampered_asset_rejected(self):
        path=self.npm();self.sums();path.write_bytes(path.read_bytes()+b'changed')
        with self.assertRaisesRegex(ValueError,'SHA256'):registry.prepare('npm')
    def test_pull_request_branch_cannot_publish(self):
        self.run['head_branch']='feature';self.npm();self.sums()
        with self.assertRaisesRegex(ValueError,'successful main'):registry.prepare('npm')
    def test_failed_ci_cannot_publish(self):
        self.run['conclusion']='failure';self.npm();self.sums()
        with self.assertRaises(ValueError):registry.prepare('npm')
    def test_python_wheel_metadata_and_filename(self):
        with zipfile.ZipFile(self.source/'ylplugin-0.1.1-py3-none-any.whl','w') as archive:archive.writestr('ylplugin-0.1.1.dist-info/METADATA','Name: ylplugin\nVersion: 0.1.1\n')
        self.sums();registry.prepare('pypi');self.assertEqual(len(list((self.root/'registry-ready').glob('*.whl'))),1)
    def test_incomplete_maven_bundle_rejected(self):
        with zipfile.ZipFile(self.source/'ylplugin-jvm-0.1.1.zip','w') as archive:
            for name in ('ylplugin','ylplugin-kotlin','ylplugin-natives'):
                pom=f'<project xmlns="http://maven.apache.org/POM/4.0.0"><groupId>xyz.younglion</groupId><artifactId>{name}</artifactId><version>0.1.1</version></project>'
                archive.writestr(f'xyz/younglion/{name}/0.1.1/{name}-0.1.1.pom',pom)
        self.sums()
        with self.assertRaisesRegex(ValueError,'incomplete Maven'):registry.prepare('maven')

if __name__=='__main__':unittest.main()
