# Third-party dependency notices

No LeoMiniGames application/SDK source is copied. Its contracts were inspected
and referenced by revision. No YLPkg or external account/payment code is bundled.

| Component | Use | Upstream licensing |
|---|---|---|
| YLSpec 0.1.2 / 0.1.4 | Existing C parser, profile registry, framing and serializer | Apache-2.0; upstream LICENSE/NOTICE and THIRD_PARTY_NOTICES.md |
| tomlc17 | Transitive, vendored inside upstream YLSpec | MIT; YLSpec vendor/tomlc17/LICENSE |
| OpenSSL 3 | EVP SHA256/SHA512, Ed25519, AES-GCM, RNG | Apache-2.0; OpenSSL LICENSE.txt/NOTICE |
| Zstandard | Record compression/decompression | Dual BSD-3-Clause / GPL-2.0; select BSD-3-Clause for distribution |
| nlohmann-json | SDK/CLI machine interfaces and host configuration | MIT; upstream LICENSE.MIT |
| Qt 6 Core (optional) | RCC memory registration | LGPL-3.0 / GPL / commercial options per Qt distribution; comply with chosen terms |

Dependencies are externally installed, except when the caller supplies a separate
YLSpec checkout for add_subdirectory. Its normal install rules include its own
license/notice files. Before distributing binaries, collect the exact licenses
and applicable notices from the actual dependency builds; this inventory is not
a substitute for their license texts. The Python binding uses Python's standard
library only and does not vendor ctypes, CPython or another container parser.

## Language binding packaging

Node-API headers follow Node.js's MIT terms; JNI headers follow OpenJDK's GPL
with Classpath Exception. Kotlin's standard library remains a Maven dependency.
Bundled native artifacts include actual collected YLSpec/tomlc17, OpenSSL, Zstd,
nlohmann-json and applicable tool/header notices. Static builds do not change
the project's own LGPL-3.0-only license, and Qt is not linked into language bundles.
