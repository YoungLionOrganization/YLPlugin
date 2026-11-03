// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

import test from 'node:test';
import assert from 'node:assert/strict';
import {readFileSync, writeFileSync, mkdtempSync, realpathSync, rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join, resolve} from 'node:path';
import {fileURLToPath} from 'node:url';
import {randomBytes, generateKeyPairSync} from 'node:crypto';
import {createRequire} from 'node:module';
import {execFileSync} from 'node:child_process';
import {Package, create, sign, version} from '@younglionorganization/ylplugin';

const root = resolve(fileURLToPath(new URL('../../', import.meta.url)));
const fixture = join(root, 'examples/artifacts/data.ylplugin');
const manifest = readFileSync(join(root, 'examples/data/plugin.ylspec'), 'utf8');
const resource = {path: 'catalog.json', file: join(root, 'examples/data/catalog.json'), media: 'application/json'};
const host = {id: 'xyz.example.data-viewer', api: '1.0', os: 'linux', arch: 'x86_64', runtimes: {data: '1.0'}, memory: 67108864};
const code = expected => error => error.code === expected;

test('a rejected native version stays rejected on subsequent calls', () => {
  const work = realpathSync(mkdtempSync(join(tmpdir(), 'ylplugin-version-')));
  try {
    const fake = join(work, 'mismatch.cjs');
    writeFileSync(fake, "exports.invoke = () => '0.0.0'; exports.open = () => ({});");
    const script = `const assert = require('node:assert/strict');
      const {Package} = require(${JSON.stringify(join(root, 'javascript/src/index.cjs'))});
      for (let i = 0; i < 2; i++) assert.throws(() => new Package('unused'), /does not match/);`;
    execFileSync(process.execPath, ['-e', script], {env: {...process.env, YLPLUGIN_NODE_PATH: fake}});
  } finally { rmSync(work, {recursive: true, force: true}); }
});

test('ESM/CJS native roundtrip, bigint permissions and deterministic builder', () => {
  const work = realpathSync(mkdtempSync(join(tmpdir(), 'ylplugin-node-')));
  try {
    assert.equal(createRequire(import.meta.url)('@younglionorganization/ylplugin').version, version);
    const p = new Package(fixture);
    try {
      p.validate(); assert.equal(JSON.parse(p.read('catalog.json')).total, 42);
      assert.equal(p.targets[1].required_permissions.value, 9223372036854775807n);
      assert.deepEqual(p.permissionBytes(0, {optional: true}), Buffer.from([37, 8, 255, 0]));
      assert.equal(p.check(host).status, 'SUPPORTED');
      const analysis = {...host, id: 'xyz.example.analysis', permission_formats: ['bitmask'],
        permission_registries: {'xyz.example.analysis.bits': '1'}, grants: {analysis: p.targets[1].required_permissions}};
      assert.equal(p.check(analysis).status, 'SUPPORTED');
      analysis.grants.analysis = {...analysis.grants.analysis, value: 37};
      assert.equal(p.check(analysis).status, 'NEEDS_PERMISSION');
      assert.equal(p.readSlice('catalog.json', 0n, 1n).toString(), '{');
      assert.throws(() => p.readSlice('catalog.json', 2n ** 64n, 1), RangeError);
      assert.throws(() => p.readSlice('catalog.json', 2 ** 53, 1), RangeError);
      assert.throws(() => p.read('catalog.json', {cancel: () => true}), code(10));
      create(join(work, 'built'), manifest, [resource]);
      assert.deepEqual(readFileSync(join(work, 'built')), readFileSync(fixture));
      const unknown = new Package(join(root, 'tests/vectors/unsupported-profile.ylplugin'));
      try { assert.throws(() => unknown.targets, code(2)); } finally { unknown.close(); }
    } finally { p.close(); }
    assert.throws(() => p.inspect(), /closed/);
    assert.throws(() => new Package(fixture, {record: 1}), code(4));
    assert.throws(() => new Package(fixture + '\0suffix'), code(3));
  } finally { rmSync(work, {recursive: true, force: true}); }
});

test('encrypted/compressed resources, callback lifetime, signatures and extraction', () => {
  const work = realpathSync(mkdtempSync(join(tmpdir(), 'ylplugin-node-')));
  try {
    const key = randomBytes(32), keys = id => id === 'content' ? key : null;
    create(join(work, 'encrypted'), manifest, [{...resource, compression: 1, key_id: 'content'}], {keys});
    const p = new Package(join(work, 'encrypted'));
    try {
      assert.throws(() => p.read('catalog.json'), code(8));
      assert.throws(() => p.read('catalog.json', {keys: () => Buffer.alloc(32, 99)}), code(6));
      assert.throws(() => p.read('catalog.json', {keys: () => { throw new Error('provider failure'); }}), /provider failure/);
      assert.equal(JSON.parse(p.read('catalog.json', {keys: () => {
        assert.throws(() => p.close(), code(3)); return key;
      }})).total, 42);
      p.validate({keys});
      if (process.platform !== 'win32') {
        p.extract(join(work, 'extracted'), [], {keys});
        assert.equal(JSON.parse(readFileSync(join(work, 'extracted/catalog.json'))).total, 42);
      }
    } finally { p.close(); }
    const pair = generateKeyPairSync('ed25519');
    const pem = Buffer.from(pair.privateKey.export({format: 'pem', type: 'pkcs8'}));
    sign(join(work, 'encrypted'), join(work, 'signed'), 'publisher', pem);
    const pub = pair.publicKey.export({format: 'der', type: 'spki'}).subarray(-32);
    const signed = new Package(join(work, 'signed'));
    try {
      signed.verify(id => id === 'publisher' ? pub : null);
      assert.throws(() => signed.verify(() => null), code(7));
    } finally { signed.close(); pem.fill(0); }
  } finally { rmSync(work, {recursive: true, force: true}); }
});

test('host-supplied WebAssembly runtime explicitly executes the verified example', () => {
  const output = execFileSync(process.execPath, [join(root, 'examples/wasm/host.mjs'), join(root, 'examples/artifacts/wasm.ylplugin')], {encoding: 'utf8'});
  assert.equal(output.trim(), '42');
});
