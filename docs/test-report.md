# Executed verification history

## 0.1.3 metadata discovery and bundled CLI — 2026-11-06 (Asia/Baku)

Implementation commit: `e1b2ed7a8638e825cbdb9989c342276703405f36`.
[SDK workflow 42125664789](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/42125664789)
passed all six jobs: Linux x86_64/ARM64, macOS x86_64/ARM64, Windows x86_64 and
aggregate packaging. Each platform installed its actual wheel and ran the core,
console command, module command, metadata and validation without SDK selectors.
The aggregate job installed the npm archive, ran Java/Kotlin with the bundled
native JAR, checked licensing/source payloads and passed Twine for all five wheels.
[Core workflow 42125664950](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/42125664950)
also passed all seven jobs, including all four desktop platforms, sanitizers,
fuzz and release-asset packaging. Two Ubuntu dependency installs downloaded Qt
packages unusually slowly; those jobs were retried without changing the source.
All thirteen final job results are successful.

This is verified build output, not a registry release.

Local Linux x86_64 checks for the development additions:

| Check | Result |
|---|---|
| Release static SDK and CTest | 9/9 suites passed |
| ASan/UBSan with Qt RCC and QML | 11/11 suites passed |
| C++ and C metadata discovery | Exact header/frame bounds, virtual 5 GiB source, corrupt TOC independence, malformed header/frame and limit checks passed |
| Python SDK | Exact-range virtual reader, exception propagation, matching indexed metadata, corrupt TOC and limit checks passed |
| Node SDK and TypeScript | 5 native suites and ESM/CJS declaration checks passed |
| Java/Kotlin | 23 Java/JNI checks with explicit native path; 24 with bundled resources/CLI; Kotlin smoke passed |
| Python platform wheel | auditwheel repaired; installed in a clean venv; Library(), console script, module CLI, metadata and validate passed without SDK selectors |
| Actual npm archive installation | Installed local bin link, native metadata/validate and bundled addon passed without SDK selectors |
| JVM distributions | Java/Kotlin/natives JARs and local staging publication built; extracted native CLI executed successfully |
| PyPI metadata | Twine wheel metadata/description check passed |
| Wiki | 24 English Markdown files and 6 offline publication/link tests passed |

A Linux syscall trace of the data example recorded reads of exactly 128, 32 and
1011 bytes from the plugin, totaling 1171 of its 1443 bytes. No TOC or payload read
was requested. OS block caching is outside this assertion. Discovery deliberately
does not validate signatures, plugin-profile semantics or the rest of the file.

The platform CI workflow additionally installs each produced wheel in a fresh
virtual environment and tests its console command. The aggregate job installs the
actual npm archive and runs Java/Kotlin against bundled native resources. These
were development artifacts for version 0.1.3; subsequent publication is recorded
in the [publishing record](publishing.md).

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
is implied. This initial report predates the owner's LGPLv3 selection in 0.1.2;
independent security review remains open.

## 0.1.1 SDK expansion — 2026-11-03

Local checks include 11/11 ASan/UBSan CTests and 9/9 static-core CTests,
ESM/CommonJS and TS declarations, four Node integration
suites, 19 Java/JNI checks, Kotlin resource/use lifecycle tests, Python integer/
limit/thread/callback ownership checks and static-core CTest. The actual data
and WASM fixtures round-trip deterministically. Native sample activation uses an
external approved digest; denied hashes do not produce a cache.

YLSpec organization-secret availability check passed in
[run 41449801522](https://github.com/YoungLionOrganization/YLSpec/actions/runs/41449801522)
without printing values or publishing. New remote platform results belong to the
language-libraries run for the exact commit; workflow definitions alone are not
reported as execution. Registry uploads were not requested or performed here.

Seven offline registry-gate tests cover checksum tampering, wrong npm owner,
failed/non-main CI, Python identity and incomplete Maven bundles. Installed
platform-wheel autodiscovery, packaged Node prebuild loading and Java/Kotlin
native-JAR loading also passed locally. No registry endpoint was used by these tests.

### Completed remote verification

The final implementation commit is `f36b5d5e5ba1e20cb060cf8a9f385dd1947310a2`:

* [Core build-and-conformance 41705359048](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/41705359048)
  passed all seven jobs, including desktop builds, sanitizers, fuzzing and source packaging.
* [Language libraries 41705359143](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/41705359143)
  passed all six jobs: Linux x86_64/ARM64, macOS x86_64/ARM64, Windows x86_64 and aggregate packaging.
  Each platform tested the native SDK APIs and installed its Python wheel.
  Windows native-plugin activation remains intentionally skipped because secure extraction is unsupported.
* [Inherited-secret check 41703864231](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/41703864231)
  passed on `1c4db135f2163e773e2d49976a3f2b635ed7b224`; the subsequent implementation change only adds checkout attributes.

The downloaded `registry-distributions` artifact (ID `13267200195`) was independently
checked against every SHA256SUMS entry. It contains five Node prebuilds, five Python
platform wheels and three Maven publications; the natives JAR contains all five JNI
runtimes. `libraries.json` identifies the exact implementation commit above.
The Windows import-library name and LF checkout policy were corrected before these
successful runs; deterministic example comparisons now pass on Windows too.

These results verify build, test, install and packaging behavior. Secret presence
does not validate registry-side ownership or token permissions. No npm, PyPI or
Central version was uploaded, and the published v0.1.0 release remains unchanged.

## 0.1.2 license and publication verification — 2026-11-04

The LGPL-3.0-only implementation commit is
`81b1fa257ca7085f41180d2f2b08934a852c1940`, authored and committed as Cavanshirpro.
GitHub's license API identifies the root LICENSE as LGPL-3.0.

* [Core CI 41709787677](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/41709787677):
  all seven jobs passed, including sanitizers, fuzzing and source/Python-facade packaging.
* [Language CI 41709787678](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/41709787678):
  five platform builds and aggregate packaging passed. Actual npm/wheel/JAR/POM
  contents were checked for LGPL metadata, full license texts and project source archives.
* Local verification passed nine static-core CTests, four Node tests, TS ESM/CJS
  declarations, 19 Java/JNI checks, Kotlin checks, seven registry gates and schema checks.
  The local wheel/npm tarball and all nine Maven JARs contained the license texts
  and project source archive; all three POMs declared LGPLv3.

Completed publication workflows:

| Destination | Successful run |
|---|---|
| GitHub v0.1.2 | [41711388362](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/41711388362) |
| npm | [41711646655](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/41711646655) |
| PyPI, five platform wheels | [41711655711](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/41711655711) |
| Maven Central, three artifacts | [41711661036](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/41711661036) |

Public npm and PyPI metadata were checked for version 0.1.2 and LGPL-3.0-only;
the three published Maven POM license declarations were checked independently.
Earlier 0.1.1 uploads also completed successfully before the owner selected LGPLv3;
their historical metadata remains immutable. Current distributions carry the selected license.

## 0.1.3 development verification — 2026-11-04

Implementation commit: `cca0c717309857d2223421e547b29bd4a077527e`.
At this historical check, 0.1.3 was unreleased and published packages were 0.1.2.

Local verification passed 11 ASan/UBSan CTests (including RCC/QML), nine
static-core CTests, five Node tests, TypeScript ESM/CommonJS checks, 19 Java/JNI
checks, Kotlin lifecycle/resource checks, seven offline registry gates, schema
checks and version/license checks. Touching VERSION caused Ninja to rerun CMake
configuration without an explicit configure command.

The Node wrong-file-open bug was reproduced on the 0.1.2 native addon: a lone
UTF-16 surrogate opened an existing U+FFFD filename. The new regression rejects
that path with code 3 and retains valid Unicode paths. Windows testing also
exposed eager local-code-page conversion in core file diagnostics; those now
use UTF-8 only when reporting an I/O failure. Regressions cover Unicode input/
output package names and a missing Unicode filename returning I/O code 5. Core regressions check
full-resource hashing, rejection of tail/out-of-slice corruption, bounded reader
requests, empty-resource cancellation and cancellation after the final callback.

The isolated 32 MiB, seven-sample warm-cache comparison measured 40.85 → 28.88 ms
median validation and 50.02 → 18.60 MiB whole-process peak RSS. This is one Linux
Release-build workload; [performance methodology and raw samples](performance.md)
describe the exact scope. It is not a cross-platform speed guarantee.

Remote verification for the implementation commit above completed successfully:

| Workflow | Result |
|---|---|
| [Core CI 41889748794](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/41889748794) | All seven jobs passed: desktop builds, sanitizers, fuzzing and packaging |
| [Language CI 41889749475](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/41889749475) | All six jobs passed: five platform builds and aggregate packaging |

The Windows SDK job passed the lone-surrogate/valid-Unicode filesystem regression,
all five Node tests, 19 Java/JNI checks, Kotlin checks and installed-wheel runtime
autodiscovery. Linux x86_64/ARM64 and macOS x86_64/ARM64 SDK jobs also passed.
Native plugin activation remains intentionally unsupported on Windows because
secure extraction is not implemented; SDK Unicode filesystem tests do execute there.

This historical check did not publish a 0.1.3 tag, release or registry version.
Its outputs were development artifacts; 0.1.2 was the latest release at the time.

## English Wiki and documentation verification — 2026-11-05

The documentation expansion adds getting-started, authoring, host-integration
and FAQ guides plus 23 English Wiki Markdown files (21 topic pages, sidebar and
footer), totaling approximately 8,200 words. Funding links use the owner's
existing Patreon configuration; the Funding commit was preserved.

Six offline Wiki-tool tests passed. They cover complete source/navigation link
resolution, Wiki/repository URL conversion, anchors/external URLs, fenced/inline
code preservation, invalid-link/fence rejection and actual local Git publication.
The Git integration test verifies preservation of unrelated owner pages/assets,
license text copying, source revision recording, Cavanshirpro attribution and
idempotent publication without an empty commit. It uses a temporary local bare
repository and does not publish to GitHub.

The exported page/license SHA256 values were checked against actual bytes. The
copyable authoring tutorial was executed through native CLI create, validation,
preflight and a Python resource read; its host was SUPPORTED and total was 42.
Relative documentation links and Python syntax checks passed.

After the owner enabled Wikis and saved the initial Home page, live publication
to `YLPlugin.wiki.git` succeeded. Wiki commit
`fbb592340946f11148f56edccb035dc09e80d816` contains all 23 rendered Markdown files,
LICENSE/COPYING and the source manifest for
`187ff9d2f24db82f63bc1f2859fd47f140ec52a9`. A fresh remote clone independently
matched every page/license SHA256 and confirmed Cavanshirpro attribution.
About/topics administration calls returned HTTP 403 through the managed
integration. The owner then applied the prepared settings through GitHub's UI.
Fresh repository readback independently matched the proposed description,
published Wiki homepage and all 15 topics. Funding still points to the owner's
Patreon configuration. Repository setup and live Wiki publication are complete;
the integration's administration restriction is retained as a capability note.
