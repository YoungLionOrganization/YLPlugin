# Python SDK

The `ylplugin` package is a typed ctypes facade over the native C ABI. It does
not contain a second container parser or a Python plugin execution engine.
Published version 0.1.2 requires Python 3.11+ and supplies platform wheels.

```sh
python -m pip install ylplugin==0.1.2
```

## Open, validate and read

```python
import json
from ylplugin import Library, Limits

sdk = Library()
with sdk.open("data.ylplugin", Limits(record=64 * 1024 * 1024)) as plugin:
    plugin.validate()
    print(json.loads(plugin.read("catalog.json"))["total"])
    print(plugin.targets)
    print(plugin.permission_bytes(0, optional=True))
```

`Library()` discovers the bundled runtime, an explicit environment selection or
an installed SDK. Use `Library('/absolute/path/libylplugin.so')` or
`YLPLUGIN_LIBRARY` for source builds. The runtime and facade version must match;
a newer source facade cannot load an older registry runtime merely because the
C ABI number has not changed.

## Operations and types

`Library.create` accepts manifest text/resource dictionaries and optional key/
cancellation providers. `Library.sign` accepts caller-owned PEM bytes.
Package operations include metadata, inspection, targets/entrypoints, read,
read_slice, permission_bytes, validate, verify, check and extract.

Metadata is public TOML text. Reports use Python dictionaries/lists. Integers
remain exact Python int values, byte permissions return bytes, and resource
reads return bytes. Decode JSON/text only when the resource's content requires
it; a binary resource is not a Unicode string.

```python
key = bytes.fromhex("00" * 32)  # illustrative test key, not a deployment key
def content_key(key_id):
    return key if key_id == "example-content" else None

with sdk.open("encrypted.ylplugin") as plugin:
    data = plugin.read("catalog.json", keys=content_key)
```

A real host obtains keys from its entitlement/key-management policy. Keys and
trust providers return exactly 32 bytes or None. Python provider exceptions
deny the key; cancellation exceptions request cancellation. Native failures are
`Error` with the C status in `code`.

## Ownership and concurrency

Use context managers or close explicitly. A handle serializes operations and
close with an RLock; reentry/close from an active callback is rejected. Avoid
cross-thread callback waits that require the same handle lock. Separate handles
are appropriate for independently scheduled I/O.

The facade validates negative/fractional/bool/overflowing ranges before ctypes
can narrow them. `read_slice(path, offset, count)` has no cancellation argument;
full reads and validation do. In development 0.1.3, plaintext validation uses
bounded streaming hash checks; full read still returns the complete resource.

See [the complete Python guide](../python-api.md),
[performance](Performance-and-Limits.md) and [troubleshooting](Troubleshooting.md).
