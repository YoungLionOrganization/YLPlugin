# Documentation guide

Start with the SDK for your host language, then use preflight and explicit host
policy before selecting a runtime. Packaging a component and authorizing it to
execute are separate decisions.

| Topic | Guide |
|---|---|
| Format, offsets, codecs and cryptographic coverage | [Specification](specification.md) |
| Profile fields, permissions, version ranges and target selection | [Plugin profile](profile.md) |
| Build dependencies, static/shared SDK and installation | [Building](building.md) |
| C/C++ ownership and callback contracts | [Public SDK](api.md) |
| Node ESM/CJS, exact integers and TypeScript | [JS/TS](javascript-api.md) |
| Java lifecycle, JNI and maps | [Java](java-api.md) |
| Kotlin `use` helpers | [Kotlin](kotlin-api.md) |
| Python lifecycle, callbacks and platform wheels | [Python](python-api.md) |
| Runnable data, WASM, native, RCC and encrypted examples | [Examples](examples.md) |
| Commands and stable exit codes | [CLI](cli.md) |
| Supported binaries versus format portability | [Compatibility](compatibility.md) |
| Adapter state machine and native/RCC trust | [Runtimes](runtime.md) |
| Threat model and FFI boundaries | [Security](security.md) |
| Organization secrets and registry publication | [Publishing](publishing.md) |
| Actual executed checks | [Test report](test-report.md) |
| Validation memory, benchmark method and measured limits | [Performance](performance.md) |
| Version mismatches, platform/argument/integrity/cancellation errors | [Troubleshooting](troubleshooting.md) |
| Development and reproducible bug reports | [Contributing](../CONTRIBUTING.md) |
| Legacy game compatibility | [RCC migration](migration-rcc.md) |
| Future installer boundary | [YLPkg](ylpkg.md) |

Container 1.0 and profile 1.0 remain independent of SDK 0.1.x. The 0.1.1 language
work does not change the wire layout, assign permission meanings or alter the
existing YLSpec/LeoMiniGames loaders.
