# Testing and Contributing

The default branch is development 0.1.3; released packages remain 0.1.2. Changes
to original material use LGPL-3.0-only and retain SPDX/dependency notices.

## Local checks by change type

| Change | Relevant checks |
|---|---|
| Container/profile/runtime | CTest, conformance vectors and schema checks |
| Ownership/buffers/callbacks/codecs | ASan/UBSan build and affected lifecycle tests |
| JS/TS | Native addon tests plus ESM/CommonJS TypeScript checks |
| Java/Kotlin | JNI build and Java/Kotlin smoke tests |
| Python | Native C ABI integration and facade tests |
| Package/version/license | Version/license checks and actual packaged metadata/content checks |
| Performance | Same-machine baseline/candidate experiment plus correctness regressions |

```sh
ctest --test-dir build --output-on-failure
python tools/check_versions.py
python tools/check_licenses.py
python tests/schema.py
python tests/test_registry.py
npm --prefix javascript run typecheck
```

Native language tests need matching explicit/bundled libraries. Follow
[Installation and Builds](Installation-and-Builds.md) rather than using an old
prebuild with a newly edited facade.

## What CI proves

The core workflow builds desktops, runs sanitizers/fuzzing and prepares source/
facade assets. The language workflow builds five desktop targets, executes APIs,
installs platform wheels and assembles npm/Maven distributions. Packaging checks
actual LGPL metadata, license texts and project sources inside archives.

0.1.3 implementation commit `cca0c717309857d2223421e547b29bd4a077527e` passed all
thirteen jobs in its [core](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/41889748794)
and [language](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/41889749475)
runs. Its Windows Unicode regression, five Node tests, 19 Java/JNI checks,
Kotlin checks and installed Python wheel all passed. This does not claim native
plugin activation on Windows or untested mobile/device support.

## Regression and benchmark expectations

Preserve complete hashing, signature coverage, permissions, limits, extraction
policy and callback ownership when optimizing. Performance tests should not
assert a machine-specific millisecond threshold. Record inputs, cache state,
compiler/build mode, sample count and memory scope. The current validation
benchmark retains raw results and distinguishes whole-process RSS from payload
scratch buffers.

Bug reports should include versions, OS/CPU/runtime, operation/error code,
expected/actual result and a minimal reproduction. See
[CONTRIBUTING.md](../../CONTRIBUTING.md), [test report](../test-report.md) and
[Performance and Limits](Performance-and-Limits.md).
