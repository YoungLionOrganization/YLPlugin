# YLPlugin for JavaScript and TypeScript

Node-API SDK for the native container, with ESM/CommonJS declarations and exact
bigint support. Configured package: `@younglionorganization/ylplugin` 0.1.1
(development; publication is manual).

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
The project's license remains pending.
