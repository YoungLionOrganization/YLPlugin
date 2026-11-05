// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

'use strict';
const {join, resolve} = require('node:path');
const metadata = require('../package.json');
let addon;
function native() {
  if (addon) return addon;
  const target = `${process.platform}-${process.arch}`;
  const location = process.env.YLPLUGIN_NODE_PATH || join(__dirname, '..', 'prebuilds', target, 'ylplugin.node');
  let loaded;
  try { loaded = require(resolve(location)); }
  catch (cause) { throw new Error(`No YLPlugin native addon for ${target}; build it or set YLPLUGIN_NODE_PATH. ${cause.message}`, {cause}); }
  const version = loaded.invoke(null, 'version', '{}', Buffer.alloc(0), undefined, undefined, undefined);
  if (version !== metadata.version) throw new Error(`YLPlugin addon ${version} does not match package ${metadata.version}`);
  addon = loaded;
  return addon;
}
function encode(value, seen = new Set()) {
  if (value === null) return 'null';
  if (typeof value === 'string') return JSON.stringify(value);
  if (typeof value === 'boolean') return value ? 'true' : 'false';
  if (typeof value === 'bigint') {
    if (value < -(1n << 63n) || value >= (1n << 64n)) throw new RangeError('JSON integer exceeds native 64-bit range');
    return value.toString(10);
  }
  if (typeof value === 'number') {
    if (!Number.isFinite(value) || (Number.isInteger(value) && !Number.isSafeInteger(value))) throw new RangeError('Use bigint for integers above the safe number range');
    return JSON.stringify(value);
  }
  if (typeof value !== 'object' || seen.has(value)) throw new TypeError('Unsupported or cyclic configuration value');
  seen.add(value);
  try {
    if (Array.isArray(value)) return '[' + value.map(item => encode(item, seen)).join(',') + ']';
    if (Object.getPrototypeOf(value) !== Object.prototype && Object.getPrototypeOf(value) !== null) throw new TypeError('Configuration must use plain objects');
    return '{' + Object.entries(value).map(([key, item]) => JSON.stringify(key) + ':' + encode(item, seen)).join(',') + '}';
  } finally { seen.delete(value); }
}
function call(handle, op, args = {}, options = {}, binary = Buffer.alloc(0)) {
  for (const name of ['keys', 'trust', 'cancel']) if (options[name] !== undefined && typeof options[name] !== 'function') throw new TypeError(`${name} must be a synchronous function`);
  return native().invoke(handle, op, encode(args), binary, options.keys, options.trust, options.cancel);
}
class Package {
  #handle;
  constructor(path, limits = {}) { this.#handle = native().open(path, encode(limits)); }
  #call(op, args, options) {
    if (!this.#handle) throw new Error('Package is closed');
    return call(this.#handle, op, args, options);
  }
  close() { if (this.#handle) { native().close(this.#handle); this.#handle = undefined; } }
  get metadata() { return this.#call('metadata'); }
  inspect() { return this.#call('inspect'); }
  get targets() { return this.#call('targets'); }
  get entrypoints() { return this.#call('entrypoints'); }
  permissionBytes(target, {optional = false} = {}) { return this.#call('permissionBytes', {target, optional}); }
  read(path, options) { return this.#call('read', {path}, options); }
  readSlice(path, offset, count, options) { return this.#call('slice', {path, offset, count}, options); }
  validate(options) { this.#call('validate', {}, options); }
  verify(trust, options = {}) { this.#call('verify', {}, {...options, trust}); }
  check(host, options) { return this.#call('check', {host}, options); }
  extract(destination, paths = [], options) { this.#call('extract', {destination, paths}, options); }
}
function create(output, manifest, resources, options) { call(null, 'create', {output, manifest, resources}, options); }
function readMetadata(path, limits = {}) { return call(null, 'readMetadata', {path, limits}); }
function sign(input, output, keyId, privatePem, options) {
  const secret = Buffer.from(privatePem);
  try { call(null, 'sign', {input, output, keyId}, options, secret); }
  finally { secret.fill(0); }
}
module.exports = {Package, create, sign, readMetadata, version: metadata.version};
