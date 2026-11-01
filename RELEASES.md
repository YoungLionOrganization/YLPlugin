# Release status

0.1.0 is the initial experimental release. Container/profile 1.0 are draft version
boundaries, not a stable-standard announcement. This source/Python-facade release
is explicitly authorized; package-registry uploads and production deployment are
outside its scope. Release notes are in docs/releases/v0.1.0.md.

Before production use or stable compatibility promises:

* Choose the project's license and supply approved LICENSE/NOTICE text.
* Independently review parser, crypto domain/coverage, extraction and ABI contracts.
* Run extended coverage-guided fuzzing and resolve all findings.
* Execute the declared desktop matrix; add Windows ARM64, macOS Intel and real
  Android/iOS SDK/device/lifecycle/low-memory tests.
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
