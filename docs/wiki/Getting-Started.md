# Getting Started

Start with a data container. It exercises real metadata, resources and hashes
without loading executable plugin code or depending on Qt.

## Obtain matching inputs

This walkthrough targets 0.1.3. Use its matching tag:

```sh
mkdir ylplugin-work
cd ylplugin-work
git clone --branch v0.1.3 --depth 1 https://github.com/YoungLionOrganization/YLPlugin.git source
```

`source/examples/artifacts/data.ylplugin` contains `catalog.json`, whose `total`
is 42. The same directory also contains real WASM and RCC fixtures. Keep those
for later; opening a data fixture requires no runtime adapter.

## Python path

Create/activate a virtual environment and install:

```sh
python -m venv .venv
# POSIX: . .venv/bin/activate
# PowerShell: .venv\Scripts\Activate.ps1
python -m pip install ylplugin==0.1.3
ylplugin --version
ylplugin metadata source/examples/artifacts/data.ylplugin --json
```

From `ylplugin-work`, save and run:

```python
import json
from ylplugin import Library

with Library().open("source/examples/artifacts/data.ylplugin") as package:
    package.validate()
    data = json.loads(package.read("catalog.json"))
    assert data["total"] == 42
    print(package.targets)
```

The platform wheel provides a matching runtime. If no compatible wheel exists,
read [Installation and Builds](Installation-and-Builds.md) rather than mixing
another platform's native library with the facade.

## Node.js path

Create a separate consumer directory:

```sh
mkdir node-client
cd node-client
npm init -y
npm install @younglionorganization/ylplugin@0.1.3
```

Save `read.mjs` and run `node read.mjs`:

```javascript
import {Package} from '@younglionorganization/ylplugin';

const packageFile = '../source/examples/artifacts/data.ylplugin';
const plugin = new Package(packageFile);
try {
  plugin.validate();
  console.log(JSON.parse(plugin.read('catalog.json')).total);
} finally {
  plugin.close();
}
```

TypeScript declarations and CommonJS exports ship in the same package. The SDK
uses Node-API; installing it does not create a browser runtime.

## What just happened?

Open parsed bounded structure/metadata. Validation checked profile and resource
integrity. Read returned hash-verified resource bytes. No publisher identity was
authenticated because this fixture is unsigned; no code was activated and no
permissions were granted.

Next, [author your own container](Authoring-Plugins.md),
[integrate a host](Host-Integration.md) or select the
[Java/Kotlin SDK](Java-and-Kotlin.md). The
[canonical walkthrough](../getting-started.md) also explains CLI inspection.
