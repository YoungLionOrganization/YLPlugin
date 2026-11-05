# Documentation guide

For **0.1.3 development**, see [metadata-only discovery](metadata-only.md)
and [bundled core/CLI installation](installation-cli.md). These additions
are absent from published 0.1.2.

## Reading paths

* First use: [getting started](getting-started.md), then the SDK guide for your language.
* Plugin author: [authoring tutorial](plugin-authoring.md), profile and examples.
* Host developer: [host integration](host-integration.md), preflight, runtime and security contracts.
* Maintainer: contributing, verification, publishing and [Wiki maintenance](wiki-publication.md).

The [published English Wiki](https://github.com/YoungLionOrganization/YLPlugin/wiki) covers these workflows as an organized
handbook with language guides, a CLI cookbook, crypto, runtime, performance,
troubleshooting, licensing and community pages. [Versioned Wiki sources](wiki/Home.md)
remain browsable in the repository. [FAQ](faq.md) answers common scope questions.

## Reference map

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
