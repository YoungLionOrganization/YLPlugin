# Python SDK

`ylplugin` requires Python 3.11+. Version 0.1.2 platform wheels
contain a ctypes runtime built from the same core. Source/facade-only installs
require an installed SDK or explicit path. Published v0.1.0's facade-only wheel
is preserved.

```python
import json
from ylplugin import Library, Limits

sdk = Library()  # bundled runtime, YLPLUGIN_LIBRARY, or installed SDK
with sdk.open("data.ylplugin", Limits(record=64 * 1024 * 1024)) as plugin:
    plugin.validate()
    print(json.loads(plugin.read("catalog.json"))["total"])
```

Use `Library('/absolute/path/libylplugin.so')` or `YLPLUGIN_LIBRARY` for local
builds. Runtime/package versions must match. Nothing is downloaded at import.
The package has a `py.typed` marker and annotations; native validation remains
authoritative.

`Library.create/sign` and `Package.metadata/inspect/targets/entrypoints/read/
read_slice/validate/verify/check/extract` call the C ABI. Metadata is TOML text;
reports are dictionaries/lists, integers preserve permission bits, resources are
bytes. The SDK does not invent meanings for permission names or bits.

Keys/trust callbacks return exactly 32 bytes or None. Callback exceptions deny
the key; cancellation exceptions cancel. `Error.code` is the native status.
Unknown-profile target/entrypoint access raises the profile error rather than
returning an empty list. Negative, bool, fractional and overflowing ranges/limits
fail before ctypes can truncate them. Paths are UTF-8 without NUL.

A handle serializes operations and close with an RLock; callback close/reentry
is rejected. Use context managers and separate handles for parallel I/O. Avoid
cross-thread lock cycles in callbacks. User-owned Python key bytes cannot promise
erasure. The SDK is a host library, not a Python execution adapter; SDK loading,
preflight and plugin activation are separate trust decisions.

`permission_bytes(target, optional=True)` copies the native byte representation
to Python bytes and rejects string/mask declarations; no decimal-string encoding is involved.
