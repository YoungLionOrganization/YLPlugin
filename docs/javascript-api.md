# JavaScript and TypeScript SDK

**npm publication status:** 0.1.3 is awaiting token write permissions. npm's public
latest is 0.1.2; new metadata/CLI APIs require the matching 0.1.3 source build until
the upload succeeds. See the [publication record](publishing.md#version-013-publication).

## Direct metadata and native CLI (0.1.3)

`readMetadata(path, limits?)` is exported in ESM, CommonJS and both TypeScript
declaration formats. It returns canonical TOML after reading only the header and
YLSpec frame, without constructing a Package or scanning TOC/payload/signature.
See [metadata-only reads](metadata-only.md) for validation and trust boundaries.

The npm `bin` entry registers `ylplugin`: use `npx ylplugin` for a local dependency,
or install globally to use npm's global bin directory. The native core and CLI
are bundled for supported targets. See [installation](installation-cli.md).


Package: `@younglionorganization/ylplugin`, version 0.1.3, under LGPL-3.0-only.
Install it from [npm](https://www.npmjs.com/package/@younglionorganization/ylplugin).
The publishing pipeline consumes verified CI artifacts without rebuilding them.

In 0.1.3, native file opens
reject unpaired UTF-16 surrogates instead of substituting U+FFFD. Valid Unicode
paths are preserved. Slice cancellation also covers empty resources and the
final reader callback. See [troubleshooting](troubleshooting.md).

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
