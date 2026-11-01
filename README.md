# YLPlugin

Experimental, standalone YoungLion plugin container and C/C++ SDK. A `.ylplugin`
file has its own binary format, an embedded **existing YLSpec frame**, an indexed
resource table and independently verifiable public metadata. It is not a renamed
ZIP, a LeoMiniGames-only loader, or a YLPkg installer.

Implemented in 0.1.0:

* Qt-free C++20 core with C ABI, C++ RAII, Python ctypes SDK and native CLI.
* `xyz.younglion.plugin` / `1.0` profile, validator, logical JSON schema and vectors.
* Store/Zstd, bounded indexed reads/slices, per-record SHA256 and controlled POSIX extraction.
* Multiple hosts/entrypoints, deterministic target/fallback selection, code-free preflight.
* String, integer-mask and real byte-buffer permissions; meanings/grants belong to the host.
* Ed25519 signatures, AES-256-GCM payload encryption and separate trust/license callbacks.
* Data runtime, optional memory RCC adapter and explicit-approval native C ABI adapter.
* Byte-preserving RCC conversion, a real RCC sample, tests, mutation fuzzing and CI definitions.

```sh
cmake -S . -B build -G Ninja -DYLPLUGIN_YLSPEC_SOURCE=../YLSpec
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
build/ylplugin inspect examples/artifacts/demo.ylplugin --json
build/ylplugin validate examples/artifacts/demo.ylplugin
build/ylplugin check examples/artifacts/demo.ylplugin --host examples/host.json
```

Dependencies and complete commands are in [building](docs/building.md). The
checked-in demo carries `game.rcc` unchanged. To package another existing RCC:

```sh
build/ylplugin convert legacy-game.rcc --manifest plugin.ylspec --output game.ylplugin
```

Metadata is public even when payloads are encrypted. `validate` checks content and
profile; `verify` checks authenticity separately. Neither signatures nor permission
declarations grant publisher/native authority. Native code is not sandboxed. The
RCC adapter requires host-reviewed/independently validated bytes before Qt parses
them, and the host owns QML engine creation, services and teardown.

v0.1.0 is an **experimental release, not production-ready**. Executed local tests
and remote CI evidence are recorded in [the test report](docs/test-report.md). Secure Windows
extraction, WASM/Python execution adapters, foreign custom runtime vtables, chunked
single-resource decoding, mobile validation and an independent security audit
remain open. The existing LeoMiniGames loaders and YLSpec sources are unchanged.
The project license has not been selected; see [LICENSING.md](LICENSING.md).

| Documentation | Purpose |
|---|---|
| [Specification](docs/specification.md) | Normative offsets, records, compression and cryptographic format |
| [Profile](docs/profile.md) | Fields, permissions, multi-host selection, extension/version policy |
| [SDK APIs](docs/api.md) | C/C++/Python ownership, errors, callbacks and concurrency |
| [CLI](docs/cli.md) | Actual commands, plans, keys and exit codes |
| [Runtime](docs/runtime.md) | Adapter lifecycle, native/RCC trust and platform limits |
| [Security](docs/security.md) | Boundaries, extraction, cryptography and operational limits |
| [Source audit](docs/source-audit.md) | Verified upstream APIs and pinned revisions |
| [RCC migration](docs/migration-rcc.md) | Additive LeoMiniGames v0.5/v0.6/v0.7 integration plan |
| [YLPkg contract](docs/ylpkg.md) | Shared substrate and separate installer responsibilities |
| [Conformance](docs/conformance.md) | Hex dump, vectors and rejection expectations |
| [Test report](docs/test-report.md) | What was actually run and remaining QA |

The GitHub release supplies source and the Python facade, with checksums and a
commit manifest. The facade requires a separately built native SDK. No package
registry upload or production deployment is included.
