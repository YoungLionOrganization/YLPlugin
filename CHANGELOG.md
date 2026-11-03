# Changelog

## 0.1.1 — in development

Added Node-API JS/TS, JNI Java/Kotlin, exact-integer reports, platform ctypes
wheels and native-JAR/npm packaging. Python now validates ranges before ctypes,
serializes close/read and blocks callback close; profile errors are not hidden
as empty arrays. Added data, host-supplied WASM, native and encrypted-data examples,
organization-secret checks and manual immutable-artifact registry workflows.
Container/profile versions remain 1.0. No new registry publication is claimed.

## 0.1.0 — experimental

Initial experimental implementation: container/profile draft 1.0, native reader/
writer, C/C++/Python SDK, preflight, permission transport, CLI, bounded compression,
cryptographic integrity/signatures/encryption, POSIX extraction, optional Qt RCC
mounting and explicit-approval native runtime. Added source audit, genuine RCC
fixture, conformance vectors, adversarial/mutation tests, sanitizer support and
CI definitions. No YLSpec or LeoMiniGames loader changes. See RELEASES.md for
outstanding release conditions; this is not a security-reviewed stable release.
