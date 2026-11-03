# YLPlugin

[![Release](https://img.shields.io/github/v/release/YoungLionOrganization/YLPlugin?label=release)](https://github.com/YoungLionOrganization/YLPlugin/releases/latest)
[![Core CI](https://github.com/YoungLionOrganization/YLPlugin/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/YoungLionOrganization/YLPlugin/actions/workflows/ci.yml)
[![Language SDKs](https://github.com/YoungLionOrganization/YLPlugin/actions/workflows/libraries.yml/badge.svg?branch=main)](https://github.com/YoungLionOrganization/YLPlugin/actions/workflows/libraries.yml)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)](docs/building.md)
[![JavaScript / TypeScript](https://img.shields.io/badge/JavaScript%20%2F%20TypeScript-Node.js%2020%2B-3178C6?logo=typescript&logoColor=white)](docs/javascript-api.md)
[![Java / Kotlin](https://img.shields.io/badge/Java%20%2F%20Kotlin-JVM%2011%2B-7F52FF?logo=kotlin&logoColor=white)](docs/java-api.md)
[![Python](https://img.shields.io/badge/Python-3.11%2B-3776AB?logo=python&logoColor=white)](docs/python-api.md)
[![Platforms](https://img.shields.io/badge/platforms-Linux%20%7C%20macOS%20%7C%20Windows-555)](docs/compatibility.md)
[![License pending](https://img.shields.io/badge/license-pending-lightgrey)](LICENSING.md)

A standalone plugin container and native SDK for applications that need to
inspect, validate, build and select components before executing them. A
`.ylplugin` has its own indexed binary format and embeds the existing YLSpec
metadata frame. It is not a renamed ZIP and does not require YLPkg or LeoMiniGames.

**Version: 0.1.1.** See [GitHub releases](https://github.com/YoungLionOrganization/YLPlugin/releases)
for verified downloads. npm, Maven Central and PyPI publication uses the separate
manual registry workflow described in [publishing](docs/publishing.md).
No stable ABI or production security audit is claimed during 0.x.

## Choose an SDK

| SDK | Implementation | Package / build |
|---|---|---|
| C / C++20 | Public C ABI and RAII handle | `YLPlugin::ylplugin`, [build instructions](docs/building.md) |
| JavaScript / TypeScript | Node-API over the same native core; ESM/CJS and declarations | `@younglionorganization/ylplugin`, [API](docs/javascript-api.md) |
| Java 11+ | JNI, `AutoCloseable`, structured maps/lists | `xyz.younglion:ylplugin`, [API](docs/java-api.md) |
| Kotlin / JVM 11+ | Idiomatic `use` helpers over the Java/JNI library | `xyz.younglion:ylplugin-kotlin`, [API](docs/kotlin-api.md) |
| Python 3.11+ | Typed ctypes facade; native runtime in platform wheels | `ylplugin`, [API](docs/python-api.md) |

The npm package and JVM native JAR collect only binaries built and tested by the
language CI matrix. Python wheels are tagged for their actual platform. Source
builds and unsupported platforms can supply an explicit native library path.
There is no second JS, Java or Python container parser and no runtime binary
download during SDK import. See [compatibility](docs/compatibility.md).

## What works

* Public YLSpec profile `xyz.younglion.plugin` / `1.0`, native validator and schema.
* Indexed store/Zstd resources, SHA256 integrity, bounded reads and verified slices.
* Multiple hosts, runtimes and entrypoints with deterministic preflight/fallback.
* Required/optional string, bitmask and byte permissions interpreted by the host.
* Ed25519 signatures, AES-256-GCM encryption and separate trust/license callbacks.
* POSIX transactional extraction, data/native adapters and optional memory RCC mounting.
* C/C++/JS/TS/Java/Kotlin/Python examples, conformance vectors and adversarial tests.

## Build and inspect

Requirements: CMake 3.26+, C++20/C11 compilers, YLSpec 0.1.2+, OpenSSL 3+, Zstd,
nlohmann-json 3.11+. Qt is optional and is not a dependency of the core or language SDKs.

```sh
cmake -S . -B build -G Ninja -DYLPLUGIN_YLSPEC_SOURCE=../YLSpec
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
build/ylplugin inspect examples/artifacts/data.ylplugin --json
build/ylplugin validate examples/artifacts/data.ylplugin
build/ylplugin check examples/artifacts/data.ylplugin --host examples/data/host.json
```

For native Node/JNI builds and platform packaging, follow [building](docs/building.md).
The existing `.rcc` conversion command remains available:

```sh
build/ylplugin convert legacy-game.rcc --manifest plugin.ylspec --output game.ylplugin
```

## More than RCC

| Example | Demonstrates |
|---|---|
| [Data catalog](examples/data/README.md) | JSON resource, multiple hosts, exact integer permissions; no Qt |
| [WebAssembly add](examples/wasm/README.md) | Verified WASM bytes and an explicit Node host runtime |
| [Native C plugin](examples/native/README.md) | Separate native ABI, external digest approval, prepare/start/stop/cleanup |
| [RCC/QML](examples/rcc/README.md) | Byte-preserving RCC conversion and optional memory-backed Qt mounting |
| [Encrypted/signed data](docs/examples.md#encrypted-and-signed-data) | Fresh keys, entitlement callback, signatures and tamper rejection |

`examples/artifacts` includes real RCC, data and WASM containers. Native examples
are generated for the actual build platform; private keys are never checked in.

## Trust boundaries

Metadata inspection and preflight execute no plugin code. `validate` checks
structure/profile/content; `verify` authenticates stored bytes separately. Neither
permission declarations nor signatures grant native/publisher authority. Hosts
must approve native code and independently validate/review RCC before Qt parses it.
Native execution is not sandboxed. The WASM example supplies its own host runtime;
there is no built-in WASM or Python execution adapter.

Secure Windows extraction, mobile device validation and an independent release
security audit remain open. SDK installation, SDK-native library loading and
plugin activation are separate operations. See [security](docs/security.md).

## Documentation and publication

Start with [the documentation guide](docs/index.md), [examples](docs/examples.md),
[CLI reference](docs/cli.md) or [the normative format](docs/specification.md).
[Publishing](docs/publishing.md) explains organization secrets, immutable artifacts,
registry scopes and the manual npm/PyPI/Maven workflows. The availability check
never prints secret values and does not publish packages.

The project's license decision is still pending: [LICENSING.md](LICENSING.md).
Third-party dependency terms remain separate. Package publication does not itself
grant a license or approve a plugin's execution.
