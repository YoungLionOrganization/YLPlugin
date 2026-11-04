# Contributing to YLPlugin

The default branch currently prepares **0.1.3**. The latest published packages
are **0.1.2**. Build the native runtime and language facade from the same version.
Changes are licensed under LGPL-3.0-only; retain SPDX notices and third-party terms.

## Reproduce and verify

Follow [building](docs/building.md) for dependencies and CMake configuration.
For a core change, run CTest with output on failure. Use an ASan/UBSan build for
buffer ownership, reader, codec or cancellation changes. For binding changes,
run the affected native language tests and TypeScript checks where applicable.
`tools/check_versions.py` and `tools/check_licenses.py` check distribution metadata.

Bug reports should include SDK/package/native versions, OS/architecture, runtime
version, the failing operation and error code, expected behavior, and a minimal
reproducer. A small generated fixture or manifest is more useful than an entire application.
Existing tests show examples for custom readers, integrity failures and callbacks.

## Performance changes

Measure a specific operation against the released baseline with the same compiler,
build mode, inputs and machine. Record wall times, sample count, cache conditions
and memory scope. [The performance guide](docs/performance.md) describes the
validation benchmark and the limits of its conclusions. Keep performance tests
independent of machine-specific speed thresholds; verify correctness under the
same workload and check failures/cancellation as well as the successful path.

## Compatibility and publishing

Container/profile 1.0, C ABI version 1 and exact permission integer/byte semantics
are independent of the SDK patch version. A change must not weaken hash, signature,
permission or extraction checks. Update the changelog and relevant API guide
when externally visible behavior changes.

Normal pushes build and test; they do not publish registry versions. The owner
starts publication separately after verification. See [publishing](docs/publishing.md).
