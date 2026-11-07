# Troubleshooting

## Start with the interface and version

Record package/native versions, OS/architecture, language runtime and operation.
This guide targets 0.1.3; check your selected release/development version. A matching
C ABI number does not allow a facade/runtime SDK version mismatch. Rebuild native
code after switching source versions and select the resulting library explicitly.

From 0.1.3, VERSION changes invalidate CMake automatically. For an older build
directory, rerun `cmake -S . -B build` before building. Confirm `ylplugin --version`
and the SDK's native version rather than relying only on a package lockfile.

## Native status codes

| Code | Meaning |
|---:|---|
| 0 | Success |
| 1 | Invalid container structure |
| 2 | Unsupported profile |
| 3 | Invalid metadata/argument |
| 4 | Resource limit |
| 5 | I/O |
| 6 | Integrity/authentication failure |
| 7 | Invalid signature/trust |
| 8 | Missing content key/license |
| 9 | Unsupported operation/codec |
| 10 | Cooperative cancellation |
| 11 | Permission/extraction policy |
| 12 | Runtime/lifecycle failure |
| 13 | Memory allocation failure |

Python Error.code, Node native error.code and Java YLPluginException.getCode()
use these statuses. The CLI has a different exit-code table. Preflight also
returns structured named statuses/candidate reasons; do not mix the three interfaces.

## Loader or platform failures

Use a matching platform wheel/npm prebuild/JNI resource. Linux manylinux_2_39
requires appropriate glibc; GNU Node/JNI also require a compatible C++ runtime.
Unsupported systems require source work. A universal Python facade wheel or
source installation needs a separately built native SDK.

Explicit choices are Library(path)/YLPLUGIN_LIBRARY, YLPLUGIN_NODE_PATH and JVM
ylplugin.native.path. The SDK does not download a substitute binary at import.
JVM extraction of its trusted runtime is separate from untrusted plugin extraction.

## Paths and ranges

Container paths follow portable ASCII rules. Filesystem paths are UTF-8 without
NUL. Development 0.1.3 rejects Node lone surrogates before substitution and fixes
Windows core diagnostics that previously converted valid Unicode through a local
code page. Valid Unicode paths remain usable; missing valid names report I/O.

Slice offsets/counts are nonnegative integers within the resource. JS needs
bigint outside the exact number range. Compressed/encrypted slices are unsupported.
Do not reinterpret numeric permission bytes as decimal strings.

## Integrity, signatures and keys

A damaged payload, wrong content key, bad GCM tag or malformed Zstd frame fails
validation. Slice reads authenticate all plaintext bytes, including bytes outside
the returned range. Signature trust is separate and requires host-supplied
trusted keys. A public publisher string is not a trusted signer.

Check exact key IDs and 32-byte key lengths. CLI raw AES keys are not hex files;
SDK trust uses raw Ed25519 public bytes, while CLI verify accepts public PEM.
The CLI has one content-key/signer slot per invocation; separate verify/validate
operations when those IDs differ.

## Cancellation and lifecycle

Cancellation is cooperative; a custom blocking reader must cooperate itself.
In 0.1.3 the C++/Node slice path checks before work and after the last callback,
including empty resources. Public C/Python/Java slice convenience methods do
not currently expose a cancellation parameter. Full read/validate interfaces
provide cancellation according to each language guide.

Never close/reenter a handle from its active callback. Do not wait in a callback
for a thread that needs the same handle lock. A stopped Session is not restarted.
For RCC cleanup, destroy all resource consumers before unmount/release.

## Get help

Use the GitHub bug-report form with a minimal manifest/resource and exact error
status. Read the [canonical troubleshooting guide](../troubleshooting.md),
[CLI reference](../cli.md) and [FAQ](FAQ.md) first so a loader/version problem is
not mistaken for a container parser failure.
