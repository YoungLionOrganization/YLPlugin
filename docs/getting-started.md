# Getting started

For **0.1.3 development**, see [metadata-only discovery](metadata-only.md)
and [bundled core/CLI installation](installation-cli.md). These additions
are absent from published 0.1.2.

This walkthrough reads a real container before introducing plugin execution.
Published packages are **0.1.2**; the default branch prepares **0.1.3**. Use a
released checkout with released packages, or build native code and facades from
the same development checkout. The container/profile version remains 1.0.

## 1. Choose a supported environment

Published SDK binaries cover Linux x86_64/ARM64, macOS x86_64/ARM64 and Windows
x86_64. Python requires 3.11+, Node requires 20+, and Java/Kotlin target JVM 11+.
Linux wheels require glibc 2.39; Node/JNI also need a compatible C++ runtime.
Check [compatibility](compatibility.md) before installing. Unsupported systems
need a source build, not a renamed wheel or a foreign-architecture binary.

## 2. Obtain a known fixture

```sh
mkdir ylplugin-work
cd ylplugin-work
git clone --branch v0.1.2 --depth 1 https://github.com/YoungLionOrganization/YLPlugin.git source
```

The data fixture is `source/examples/artifacts/data.ylplugin`. It contains
`catalog.json` with `total` equal to 42. It needs neither Qt nor a plugin runtime.
The source repository also includes WASM and RCC containers; use data first.

## 3. Read with Python

Create and activate a virtual environment, then install the matching package:

```sh
python -m venv .venv
# POSIX: . .venv/bin/activate
# PowerShell: .venv\Scripts\Activate.ps1
python -m pip install ylplugin==0.1.2
```

Run from `ylplugin-work`:

```python
import json
from ylplugin import Library

with Library().open("source/examples/artifacts/data.ylplugin") as plugin:
    plugin.validate()
    catalog = json.loads(plugin.read("catalog.json"))
    assert catalog["total"] == 42
    print(plugin.inspect())
```

A matching platform wheel carries its native runtime. A source installation or
the universal facade wheel needs an explicit SDK library; see [building](building.md).

## 4. Read with Node.js

Keep the consumer package separate from YLPlugin's own source package:

```sh
mkdir node-client
cd node-client
npm init -y
npm install @younglionorganization/ylplugin@0.1.2
```

Save this as `read.mjs` inside `node-client` and run `node read.mjs`:

```javascript
import {Package} from '@younglionorganization/ylplugin';

const plugin = new Package('../source/examples/artifacts/data.ylplugin');
try {
  plugin.validate();
  const catalog = JSON.parse(plugin.read('catalog.json'));
  console.log(catalog.total); // 42
} finally {
  plugin.close();
}
```

The same npm package provides CommonJS and TypeScript declarations. Browser
execution is not supplied by this Node-API package.

## 5. Inspect with a source-built CLI

Follow [building](building.md), then use the CLI from that build:

```sh
build/ylplugin inspect examples/artifacts/data.ylplugin --json
build/ylplugin validate examples/artifacts/data.ylplugin
build/ylplugin check examples/artifacts/data.ylplugin --host examples/data/host.json
```

These paths assume the shell is now at the source checkout root. On multi-config
generators, select the executable under the appropriate configuration directory.
An inspection report is metadata; it does not authenticate the publisher or run
an entrypoint. Validation checks resource integrity, while signature verification
and host policy are separate decisions.

## Continue by role

* Authors: [build your own data plugin](plugin-authoring.md).
* Host developers: [integrate preflight and a session](host-integration.md).
* JVM consumers: [Java](java-api.md), [Kotlin](kotlin-api.md) and the [Gradle guide](../jvm/README.md).
* Maintainers: [testing](../CONTRIBUTING.md), [performance](performance.md) and [publishing](publishing.md).
* Errors: [troubleshooting](troubleshooting.md) and [FAQ](faq.md).
