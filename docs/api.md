# SDK, ownership and bindings

## Metadata discovery (0.1.3 development)

`ylp::read_metadata(path, limits)` and its `(size, reader, limits)` overload read
only the fixed header and YLSpec frame. The C ABI exposes `ylp_read_metadata` and
`ylp_read_metadata_reader`; release output with `ylp_buffer_free`. No Package is
created and no TOC, payload or signature region is read. Unknown profiles are
inert readable data. See [metadata-only reads](metadata-only.md) for exact bounds,
callback lifetimes, errors and the distinction from validation.


## C ABI

Include `ylplugin/ylplugin.h`, link `YLPlugin::ylplugin`. ABI version is 1 within
experimental SDK 0.1.x; no stable 1.x ABI is promised. Versioned configuration/view structures have
`struct_size` and `abi_version`; initialize them as documented. Functions returning
`ylp_status` catch C++ exceptions, report a bounded caller-owned `ylp_error`, and
return status 0 on success. No thread-local/global last-error state is used.

| Operation | API |
|---|---|
| Open/close | `ylp_open`, `ylp_open_reader`, `ylp_close` |
| Metadata/inspection | `ylp_metadata`, `ylp_inspect` |
| Register/validate profile | `ylp_plugin_profile`, `ylp_validate_profile` |
| Resource views | `ylp_resource_count`, `ylp_resource_at` |
| Targets/entrypoints | `ylp_target_count`, `ylp_target_at`, entrypoint equivalents |
| Permissions | `ylp_permission_at`, `ylp_permission_string_count/at` |
| Read/slice | `ylp_read`, `ylp_read_slice` |
| Integrity/authenticity | `ylp_validate`, `ylp_verify` |
| Preflight | `ylp_check` (host JSON, separate key/trust callbacks) |
| Extract/build/sign | `ylp_extract`, `ylp_create`, `ylp_sign` |
| Data runtime lifecycle | `ylp_data_session`, `ylp_session_prepare/activate/start/stop/free` |

`ylp_buffer` owns native allocation. Free it exactly once with `ylp_buffer_free`,
never a foreign allocator. Outputs must be empty before a call; buffer-producing
functions initialize them empty on failure. Buffer size excludes a convenience
NUL terminator; binary bytes may contain NUL. `ylp_resource` and `ylp_permission`
string/byte pointers are borrowed until package close. Permission bytes are actual
numeric `uint8_t` values. Targets/entrypoints are returned as UTF-8 JSON for an
extensible machine interface; masks are numeric signed-64-compatible values, not
hexadecimal or strings. Counts return zero on invalid profile; call profile/
validation or indexed APIs for an error diagnostic. Out-of-range indexed access
fails. Optional permission selector is zero for required, nonzero for optional.

Paths passed to the C ABI are NUL-terminated UTF-8. Callback contexts remain
caller-owned, live through the synchronous operation (for custom readers, through
package/session lifetime). Read callbacks fill exactly the requested span and
return 0. Key/trust callbacks fill 32 bytes and return 0; nonzero means unavailable
or revoked/denied. Cancellation callbacks return nonzero to cancel. Callbacks
must not throw C++ exceptions through foreign code or free the active handle.
They must cooperate with cancellation for any blocking host I/O. Independent
operations may run concurrently; do not close/free a handle during a call.

The C runtime helpers expose the memory data adapter only. Custom and native
adapters use the C++ interface; a generic foreign runtime vtable ABI is deferred.
This release's C data-session helper does not accept encryption/trust callbacks;
use C++ Session for protected runtime activation, or C read/check APIs for data.

## C++20

`ylp::Package` is an inexpensive shared-ownership immutable RAII handle. It retains
the open reader; its metadata and resource references remain valid while any
copy keeps the implementation alive. Metadata profile validation is lazy and
thread-safe. `read` authenticates/decompresses/hashes one bounded record;
`read_slice` streams through an entire plaintext store record to verify its hash
while retaining only the requested slice. It is not a zero-copy mmap API and
does not claim constant-time integrity validation of a slice.

In 0.1.3, `validate` also uses full-resource streaming SHA256 for plaintext store
records with at most 1 MiB of payload scratch space. It does not retain successful
hashes across calls: later modifications are detected on the next scan. Encrypted/
compressed records keep the decode/authentication path. `read_slice` checks
cancellation before work and after the final reader callback, including empty
resources. See [performance](performance.md) for allocation scope and measurement.

`create` sorts resource inputs and uses the real YLSpec serializer to normalize
numeric output. `sign` accepts a caller-owned unencrypted Ed25519 private PEM
buffer and signs the stored representation. `verify` uses raw 32-byte public keys.
Host callbacks own key storage/revocation/rotation policy. `Error::code` preserves
the same numeric status ordering as the C API. No exceptions cross the C ABI;
C++ callers receive typed exceptions.

Host JSON is illustrated by `examples/host.json`. `runtimes` maps IDs to numeric
version strings; `permission_registries` maps registry IDs to exact versions;
`permission_formats` lists `strings`, `bitmask`, `bytes`; `grants` maps target IDs
to corresponding permission values. `memory` and `storage` are explicit budgets;
zero means unknown, not unlimited. Resource-dependent requirements that cannot
be evaluated return UNKNOWN. The selected record's decoded size is a lower bound
on memory demand; adapter/native/Qt overhead remains host-owned. Preflight does
not reserve resources or decrypt/execute payload code.

Preflight statuses: SUPPORTED, UNSUPPORTED, UNKNOWN, NEEDS_RUNTIME,
NEEDS_PERMISSION, NEEDS_LICENSE, NEEDS_EXTRACTION, INVALID_SIGNATURE,
INVALID_CONTAINER, RESOURCE_LIMIT, UNSUPPORTED_PROFILE. A file that cannot open
returns a C/C++ container error before preflight; the CLI maps that to its invalid
input exit code. A SUPPORTED result is not a runtime success guarantee.

## Python (CPython 3.11+)

`bindings/python/ylplugin` uses ctypes over the same C ABI. It never implements
another container parser. Install with `python -m pip install .` after building
the native SDK, then explicitly pass the library path or set YLPLUGIN_LIBRARY.
No binary is downloaded automatically.

```python
from ylplugin import Library

sdk = Library("/absolute/path/to/libylplugin.so")  # dylib / DLL on other OSes
with sdk.open("demo.ylplugin") as package:
    print(package.metadata)  # public TOML, even for encrypted resources
    print(package.targets)
    package.validate()
    rcc = package.read("game.rcc")
```

`Library.create`, `Library.sign`, `Package.inspect/validate/verify/check/read/
read_slice/extract` call native functions. `keys`/`trust` callbacks return exactly
32 bytes or None. Callback exceptions are treated as denial; cancellation callback
exceptions cancel. `Error.code` is the native status. Use a context manager or
close explicitly; never close while another thread is inside a call. No Python
execution runtime adapter is implemented. Python SDK and native runtime support
are separate concerns.

## 0.1.1 language frontends

The C ABI stays version 1. Node/JNI use a shared internal dispatcher into the
same core; Python uses the C ABI and version-checked native runtime. Native
integers are not rounded through JS doubles: large results become bigint, Java
uses Long/BigInteger and Python uses int. Per-handle ownership prevents callback
close/reentry. See [JS/TS](javascript-api.md), [Java](java-api.md),
[Kotlin](kotlin-api.md) and [Python](python-api.md) for actual frontend coverage.
