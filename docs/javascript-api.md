# JavaScript and TypeScript SDK

Package: `@younglionorganization/ylplugin`. Version 0.1.1 was accepted by npm through
the [publishing workflow](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/41708369276).
New packages can take a few minutes to become visible while npm processes them.

Node 20+ is the supported runtime. ESM and CommonJS share one implementation and
ship declarations for both module systems. Browser execution is not provided:
this SDK uses Node-API and native file access. WASM plugin bytes can be handled
by a separately supplied host runtime.

```js
import {Package} from '@younglionorganization/ylplugin';
const plugin = new Package('data.ylplugin');
try {
  plugin.validate();
  const catalog = JSON.parse(plugin.read('catalog.json'));
  console.log(catalog.total);
} finally { plugin.close(); }
```

CommonJS uses `const {Package} = require('@younglionorganization/ylplugin')`.
Exports also include `create`, `sign` and `version`. Package methods expose
metadata, inspection, targets, entrypoints, read/readSlice, validation, signature
verification, preflight and selected extraction. Use `Options.keys/trust/cancel`
for synchronous callbacks. Keys/trust return exactly 32 bytes or null/undefined.
Callbacks must not return promises; asynchronous I/O is not supported by this API.
Methods block the calling thread; use a Node worker for expensive host operations.

Integers up to JavaScript's safe range are numbers. Larger returned integers are
`bigint`; masks such as `9223372036854775807n` retain all bits. Inputs reject unsafe
integer numbers and integers beyond the native signed/unsigned 64-bit range.
No decimal permission is silently converted to a string. Configuration JSON is
an internal binding transport, not an alternative container parser.

```ts
import {Package, type Host} from '@younglionorganization/ylplugin';
const host: Host = {id: 'example.host', api: '1.0', os: 'linux', arch: 'arm64',
  runtimes: {data: '1.0'}, memory: 67108864};
const plugin = new Package('data.ylplugin', {record: 67108864n});
try { console.log(plugin.check(host).status); }
finally { plugin.close(); }
```

The native addon loads from `prebuilds/<node-platform>-<node-arch>/ylplugin.node`.
For local builds set `YLPLUGIN_NODE_PATH` to the compiled addon. A package/addon
version mismatch fails before use. There are no install scripts or import-time
downloads. Unsupported architectures/libc versions require a source build.

Explicit close is recommended; a native finalizer is fallback cleanup. Closing
or reentering a package from a callback throws rather than freeing an active
handle. Callback exceptions propagate. Native errors carry numeric `error.code`
matching the C SDK. Separate handles can be used in separate workers; a handle
itself is not transferable.

`permissionBytes(target, {optional: true})` returns a copied Buffer for a byte
permission declaration and rejects other representations. No bit meanings are assigned.
