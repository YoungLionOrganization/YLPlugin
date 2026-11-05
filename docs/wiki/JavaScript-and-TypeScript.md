# JavaScript and TypeScript

**0.1.3 development additions:** [direct metadata-only reads](Metadata-Only-Reads.md)
request only the 128-byte header and YLSpec frame. TOC, payload and signature
regions are not read. Discovery returns unverified metadata; validation and host
preflight remain separate. Python/npm now bundle the native CLI alongside the
core; Java/Kotlin supply a Java CLI entry point. See
[core/CLI installation and PATH behavior](../installation-cli.md).
These additions are not available in the published 0.1.2 packages.

Install the published desktop Node-API package:

```sh
npm install @younglionorganization/ylplugin@0.1.2
```

Node 20+ is required. ESM, CommonJS and declarations share the same implementation.
This package is not YLSpec's browser/WASM package and does not supply a browser
YLPlugin parser or automatically execute JavaScript plugins.

## ESM and CommonJS

```javascript
import {Package} from '@younglionorganization/ylplugin';

const plugin = new Package('data.ylplugin');
try {
  plugin.validate();
  console.log(JSON.parse(plugin.read('catalog.json')).total);
  console.log(plugin.permissionBytes(0, {optional: true}));
} finally {
  plugin.close();
}
```

CommonJS uses `const {Package} = require('@younglionorganization/ylplugin')`.
`create`, `sign` and `version` are also exported. Package operations include
metadata, inspection, targets/entrypoints, read/readSlice, validation, signature
verification, preflight and selected extraction.

## TypeScript and exact integers

```typescript
import {Package, type Host} from '@younglionorganization/ylplugin';

const host: Host = {
  id: 'xyz.example.data-viewer', api: '1.0', os: 'linux', arch: 'x86_64',
  runtimes: {data: '1.0'}, memory: 67108864
};
const plugin = new Package('data.ylplugin', {record: 67108864n});
try { console.log(plugin.check(host).status); }
finally { plugin.close(); }
```

Safe integer results use number; larger native integer results use bigint.
Use bigint for inputs beyond JavaScript's exact number range. Profile bitmasks
remain nonnegative signed-64 values. Byte permissions/resources are Buffers,
not decimal strings or text encodings of bytes. Type declarations are provided
for both module systems.

## Synchronous callbacks and loader behavior

Read/build/check options accept synchronous key, trust and cancellation callbacks.
Keys/trust return exactly 32 bytes or null/undefined. Do not return promises.
Methods block their calling thread; use a Node worker for expensive host tasks.
Callback exceptions propagate and cannot be used to close/reenter an active handle.

The addon loads from `prebuilds/<node-platform>-<node-arch>/ylplugin.node`, or
`YLPLUGIN_NODE_PATH` for explicit builds. Native/package version mismatches fail.
There are no installation scripts or import-time native downloads. A finalizer
is fallback cleanup; explicit close gives predictable resource lifetime.

## Development 0.1.3 behavior

The development branch rejects lone UTF-16 surrogates in file-open paths before
V8 can replace them with U+FFFD. It preserves valid Unicode paths and uses UTF-8
diagnostics on Windows. These are development fixes, not claims about the already
published 0.1.2 files. Slice cancellation now covers empty resources and the
last reader callback; complete-resource hashing is retained.

See [the API guide](../javascript-api.md) and the
[host-supplied WASM example](../../examples/wasm/README.md).
