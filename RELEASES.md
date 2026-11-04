# Release status

The latest published release is **0.1.2**, licensed under LGPL-3.0-only and
available on GitHub, npm, PyPI and Maven Central. See [release notes](docs/releases/v0.1.2.md)
and [verification evidence](docs/test-report.md). Container/profile 1.0 are
experimental boundaries, not a stable-standard announcement.

The default branch prepares **0.1.3**, with bounded-memory plaintext validation,
cancellation/Unicode fixes and incremental build versioning. It is not published.
See the [changelog](CHANGELOG.md) and [performance measurements](docs/performance.md).

Before production use or stable compatibility promises:

* Independently review parser, crypto domain/coverage, extraction and ABI contracts.
* Run extended coverage-guided fuzzing and resolve all findings.
* Extend executed desktop coverage to Windows ARM64 and actual Android/iOS
  SDK/device/lifecycle/low-memory tests. Desktop x86_64/ARM64 coverage is recorded
  for the exact released CI runs, including macOS Intel.
* Implement safe Windows extraction before enabling its native adapter.
* Validate actual host integration with retained v0.5/v0.6/v0.7 RCC fixtures and
  the existing publisher/native trust checks; the migration document alone is
  not evidence that the unmodified LeoMiniGames executable accepts YLPlugin.
* Decide compatibility policy for deferred chunking, runtime/codec extensions and
  a future separately exported shared substrate without changing wire IDs silently.
* Package reviewed dependencies, licenses, symbols and SDK consumers for each OS.

WASM/Python execution, a general C runtime-vtable registration ABI, key hardware
integration, power-loss recovery and YLPkg installation remain separate work.
No placeholder CLI commands claim these features.

## Next SDK release: 0.1.3

VERSION and mirrored language metadata describe development 0.1.3. Published
versions remain immutable. The current fixes preserve container/profile 1.0,
C ABI version 1 and existing license/package identities. Use the
[publication sequence](docs/publishing.md) after review; a normal CI run does
not publish a release or upload registry versions.
