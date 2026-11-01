# Executed test and internal audit report — 2026-11-01–02 (Asia/Baku)

Environment: Linux x86_64, GCC/G++ 14.2, CMake 3.31.6, CPython 3.12.14,
OpenSSL 3.5.7 runtime, Zstd 1.5.7, nlohmann-json 3.11.3, Qt 6.8.2.
Upstream revisions are recorded in source-audit.md. Only YLPlugin source was
changed; YLSpec and LeoMiniGames worktrees remained clean.

## Executed results

| Check | Result |
|---|---|
| Debug build, optional Qt/RCC enabled | Passed |
| Release build without Qt | 7/7 passed; core/CLI/Python linked without Qt |
| CTest normal build | 9/9 passed: seven YLPlugin suites and two upstream YLSpec suites |
| ASan + UBSan build, including YLSpec C code | 9/9 passed |
| Native core adversarial/round-trip suite | 181 checks passed; includes 3000 deterministic binary mutations |
| Native shared-library lifecycle | 8 checks passed, actual approved fixture loaded/start/stop/unload |
| CLI/Python integration | 25 counted command/error checks plus cross-language byte/assertion checks passed |
| Public conformance vectors | Seven binary vectors and positive/negative profile vectors passed |
| Qt memory RCC lifecycle | Ten mount/read/teardown cycles, held QFile lifetime, host denial passed |
| QML engine lifecycle | Five actual QML creation/property/teardown cycles passed, no retained buffers |
| JSON Schema | Draft schema checked; example and positive/negative profile projections passed |
| SDK install + separate consumer project | C/C++ consumers compiled; actual RCC resource read successfully |
| Python facade wheel | Local `ylplugin-0.1.0-py3-none-any.whl` built, no native binary bundled |
| Premium-content example | Fresh keys generated privately; encrypt/sign/verify/decrypt-validate succeeded |
| Real CLI SIGINT cancellation | Exit 8 and JSON error; no published file or leaked staging directory |

Native sanitizer tests ran with leak detection enabled and halt-on-error for
address/undefined behavior errors. ctypes/Python tests ran with the same native
address/UB checks but interpreter leak detection disabled. GCC's libasan and
libstdc++ were preloaded for the Python harness, now configured by CMake.

Large-offset coverage uses a virtual seekable file with a second TOC resource
beyond 4 GiB, without allocating a multi-GiB payload. This is not a claim of a
physical multi-GiB build/read benchmark. Mutations are deterministic smoke
fuzzing, not extended coverage-guided security certification. The Clang libFuzzer
target and a 60-second CI job are supplied; that engine/remote CI was not run
locally. Compiler warnings were enabled; no separate static-analysis tool was run.

## Role-based internal review

The requested Developer → Architect → Security/Red Team → Bug Hunter → Fix →
Platform QA → Product QA → Regression sequence was performed as an internal
self-review, not represented as an independent external audit or separate human
reviewers. Concrete outcomes:

| Review area | Finding / implemented response / evidence |
|---|---|
| Architecture | Keep YLSpec unchanged, plugin profile separate, YLPkg future installer separate; source audit, profile callback and standalone SDK consumer |
| Parser/red team | Reject integer overflow, noncanonical overlapping regions, unsafe aliases/traversal and resource bombs; adversarial vectors/core suite |
| Extraction/red team | A writable shared parent could allow staging-name replacement; require owned non-shared writable parent, anchored descriptors, O_NOFOLLOW/O_EXCL and atomic no-replace publication; symlink/shared-parent/interruption tests |
| Crypto/red team | Bind record path/type/variant/key ID/size/hash/nonce via GCM AAD; bind all stored bytes + signer descriptor via signature prefix; wrong-key/tag/path/payload/metadata/signer tampering tests |
| Bug hunting | Enforce ordered numeric version ranges; check actual adapter version; refuse extraction-policy violations; unsupported permission registry is never treated supported merely because its mask is zero |
| Lifecycle | Respect real Qt buffer lifetime, retain on busy unregister, destroy QML consumers before unmount; actual QFile and QML tests |
| Host trust | Native default deny plus separate digest-bound callback; no plugin execution during preflight/preparation; approval denial and native lifecycle tests |
| Product QA | Actual create/convert/inspect/list/validate/check/extract/sign/verify/encrypt workflows; same bytes through C/C++/Python; private-key examples do not enter source control |
| Regression | Rebuilt/re-ran after fixes, full normal and sanitized suites passed; unchanged upstream YLSpec tests also passed |

## Initial local verification limits

At the initial local verification stage, Windows/macOS/ARM/mobile execution,
signed app bundles, mobile stores, physical
device lifecycle/low-memory testing, a real integration into the current
LeoMiniGames executable, and its complete historical regression suite were not
run. Their source was inspected for compatibility; the migration plan is additive.
CI definitions do not establish platform success. Secure Windows extraction,
WASM/Python execution adapters, a general foreign runtime-vtable ABI and chunked
single-resource decoding remain unimplemented. Qt's RCC internal parser is not
claimed hardened; the adapter requires host approval/independent validation.
Project licensing and independent release/security audit remain open.

That local stage did not publish a GitHub Release, registry package or production
deployment.

## GitHub release verification

The explicitly authorized v0.1.0 publication follows the remote desktop,
sanitizer and coverage-guided fuzz workflow. The package job depends on all three
job groups succeeding, and records the exact commit and workflow URL in the
release asset `release-manifest.json`. Consult that run for the final platform
outcomes, including explicitly skipped native execution on non-Linux runners;
the initial local results above are not substituted for remote results.

Release assets contain committed sources, a pure Python facade wheel, the RCC
demo and checksums. No native binary, payment service or production deployment
is implied. License selection and independent security review remain open.
