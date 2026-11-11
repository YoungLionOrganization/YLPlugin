# YLPlugin for JavaScript and TypeScript

**npm publication status:** 0.1.3 is awaiting token write permissions. npm's public
latest is 0.1.2; new metadata/CLI APIs require the matching 0.1.3 source build until
the upload succeeds. See the [publication record](../docs/publishing.md#version-013-publication).

For **0.1.3**, see [metadata-only discovery](../docs/metadata-only.md)
and [bundled core/CLI installation](../docs/installation-cli.md). These additions
are absent from 0.1.2.

Node-API SDK for the native container, with ESM/CommonJS declarations and exact
bigint support. Package: `@younglionorganization/ylplugin` 0.1.3. Publication uses verified CI
artifacts through the manual release pipeline.

```js
import {Package} from '@younglionorganization/ylplugin';
const plugin = new Package('data.ylplugin');
try { plugin.validate(); console.log(JSON.parse(plugin.read('catalog.json')).total); }
finally { plugin.close(); }
```

Build with `YLPLUGIN_BUILD_NODE=ON` and `NODE_INCLUDE_DIR`. For local tests:

```sh
npm ci --prefix javascript --ignore-scripts
YLPLUGIN_NODE_PATH=/absolute/path/ylplugin.node npm --prefix javascript test
npm --prefix javascript run typecheck
```

Prebuilds come from the language matrix; unsupported OS/libc combinations need
source builds. There are no install hooks or downloads. Read [the API](../docs/javascript-api.md),
[compatibility](../docs/compatibility.md) and [publishing](../docs/publishing.md).
The project is licensed under LGPL-3.0-only. Packages include LICENSE, COPYING,
source archives and third-party notices; see ../LICENSING.md and ../SOURCES.md.
