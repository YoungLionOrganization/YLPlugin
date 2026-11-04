# Troubleshooting

## Native and package versions differ

The C runtime, Node addon and JNI library must match the facade's SDK version.
The published packages are 0.1.2; the current source tree prepares 0.1.3.
Rebuild native code after changing branches or versions, then select that build
using `YLPLUGIN_LIBRARY`, `YLPLUGIN_NODE_PATH` or `ylplugin.native.path`.
From 0.1.3, changing `VERSION` automatically invalidates CMake configuration.
For an older build directory run `cmake -S . -B build` before rebuilding.

Do not mix a newly built 0.1.3 facade with a 0.1.2 runtime, even when the file
format and C ABI version are unchanged. See [compatibility](compatibility.md).

## No native binary for the current platform

Published platform wheels include the ctypes runtime. The universal facade wheel
and a Python source install need a separately built shared SDK. Node packages
and the JVM natives JAR cover the platforms listed in the compatibility guide.
Unsupported architectures/libc versions require a source build.

Linux binaries are built on Ubuntu 24.04. A manylinux_2_39 wheel does not support
older glibc merely because the machine is also Linux. Node/JNI also require a
compatible C++ runtime. SDK imports do not download replacement native code.

## Invalid path or argument — code 3

Container resource names follow the portable-path rules in the specification;
filesystem paths are UTF-8 and cannot contain NUL. Node 0.1.3 also rejects lone
UTF-16 surrogates before encoding a file-open path. Valid paired Unicode and
actual U+FFFD characters are allowed in filesystem paths.

Slice offsets/counts must be nonnegative integers within the resource. JS callers
must use bigint for integers outside the safe number range. Read slices require
plaintext store resources; compressed/encrypted slices are unsupported.

## Resource limit — code 4

Compare metadata/TOC/record/count/aggregate/decompression-ratio limits with the
input. Raise only the relevant limit for a trusted workload. A larger record
limit does not imply the decoder can process encrypted/compressed data without
record-sized buffers. See [performance](performance.md).

## Integrity failure — code 6

Read/validation checks the entire plaintext resource hash. Reading a small slice
still authenticates bytes outside that slice. A damaged payload, wrong AES key,
invalid GCM tag, or malformed Zstd frame fails integrity checks. Signature trust
is a separate operation using an explicit trust provider.

## Cancellation — code 10

Callbacks are cooperative and synchronous. Return true/nonzero to cancel, and
make custom blocking readers cooperate with the host's cancellation mechanism.
From 0.1.3, C++/Node/JVM slices check cancellation before any work and after the
last reader callback, including empty resources. The C ABI's slice function has
no cancellation argument; C/Python callers can cancel full validation/read operations.

## Preflight rejects activation

Use the report's candidate reasons: check host ID/API, runtime version, OS,
architecture/ABI, permission representation and grants, extraction policy,
signature policy and explicit memory/storage budgets. Installing a language SDK
does not itself register a plugin execution runtime.

## Publishing or GitHub metadata returns 403

Registry token scope/ownership is separate from GitHub Actions secret presence.
Repository About/website/topics need administration write access; code-push
permission alone is insufficient. The prepared About values are in
[repository page settings](repository-page.md). See [publishing](publishing.md)
for the registry workflows and their provenance checks.
