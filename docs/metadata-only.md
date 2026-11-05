# Read YLSpec metadata without scanning a plugin

Available in development version **0.1.3**. Published **0.1.2** exposes metadata
through an indexed `Package` open; that operation also reads the table of contents
(TOC), alignment padding and signature descriptor when present. Use the new
standalone operations below when only plugin information is needed.

A `.ylplugin` begins with a fixed 128-byte header. The header locates a genuine
YLSpec frame at offset 128. Discovery reads that header, the 32-byte YLSpec frame
header, and exactly the declared metadata document bytes. It never opens a
`Package`, scans the TOC, reads payload data, decompresses a resource, extracts a
file or reads the signature region. Its application read volume is
`128 + metadata_frame_length`, independent of payload size.

A file handle is opened to read these ranges and obtain the file length. This is
not a claim that an operating system can read bytes without opening a file, or
that physical storage cannot perform block caching/read-ahead. The local reader
disables stdio buffering so the application does not prefetch adjacent regions.
For custom sources, the host is responsible for fetching only the requested
ranges and providing a consistent snapshot and correct total size.

## CLI

```sh
ylplugin metadata package.ylplugin
ylplugin metadata package.ylplugin --json
```

Default output is canonical UTF-8 TOML produced by YLSpec. It is semantically the
embedded document, not necessarily the original whitespace or byte sequence.
JSON output explicitly records the scope:

```json
{"scope":"metadata-only","integrity_status":"UNVERIFIED","metadata":"...TOML..."}
```

`inspect` retains its richer indexed report. `metadata` does not list resources,
validate references to resources, or infer supported targets from an unvalidated
profile. Discovery does not need a decryption key: YLSpec metadata is public even
when resources are encrypted.

## Python

```python
import tomllib
from ylplugin import Library, Limits

sdk = Library()
text = sdk.read_metadata("package.ylplugin", Limits(metadata=1024 * 1024))
document = tomllib.loads(text)
print(document["data"]["id"], document["data"]["version"])
```

A synchronous custom range reader also works without a local plugin copy:

```python
# size is the exact object length; fetch_range returns bytes for [offset, offset+count).
# Implement transport/authentication, snapshot consistency and timeouts in your host.
text = sdk.read_metadata_reader(size, fetch_range, Limits(metadata=1024 * 1024))
```

The callback must return exactly `count` bytes. Callback exceptions propagate to
the caller. No callback or file handle is retained after this operation.

## JavaScript and TypeScript

```javascript
import {readMetadata} from '@younglionorganization/ylplugin';
const toml = readMetadata('package.ylplugin', {metadata: 1024 * 1024});
```

CommonJS exports the same function and declarations. The SDK returns a TOML
string; choose a TOML parser if the host needs a structured object.

## Java and Kotlin

```java
String toml = YLPlugin.readMetadata("package.ylplugin", Map.of("metadata", 1048576));
```

```kotlin
import xyz.younglion.ylplugin.kotlin.readMetadata
val toml = readMetadata(Path.of("package.ylplugin"))
```

Java's overload accepting limits is also directly available from Kotlin.

## C and C++

```cpp
#include <ylplugin/ylplugin.hpp>
auto toml = ylp::read_metadata("package.ylplugin");
// Or: ylp::read_metadata(total_size, read_at_callback, limits).
// The callback receives (uint64_t offset, std::span<uint8_t> destination).
```

```c
#include <ylplugin/ylplugin.h>
ylp_buffer text = {0};
ylp_error error = {0};
if (ylp_read_metadata("package.ylplugin", NULL, &text, &error) == YLP_OK) {
    /* text.data holds text.size bytes of UTF-8 TOML. */
}
ylp_buffer_free(&text);
```

`ylp_read_metadata_reader(size, reader, context, limits, &text, &error)` accepts the
same exact-range callback convention as `ylp_open_reader`, but retains nothing.
Callbacks return zero after filling the requested buffer and must not throw
through the C ABI. Buffer ownership and errors follow the [C API](api.md).

## Validation and trust

Discovery checks magic, supported header version/feature flags, declared file
length, reserved header bytes, metadata locator bounds, the metadata byte limit
(default 16 MiB), and the YLSpec frame/document syntax through the native YLSpec
parser. Unknown profiles remain readable as inert metadata.

Successful discovery does **not** establish whole-container validity, plugin
profile validity, resource existence, payload integrity, signature authenticity,
permission grants or runtime compatibility. A corrupt TOC or payload can coexist
with readable metadata. Treat IDs, publisher names and descriptions as untrusted
claims until your host applies its normal verification policy.

For activation: open a `Package`, validate resources, verify a signature against
host-owned trust keys when required, run preflight, then use an approved runtime
adapter. See [host integration](host-integration.md) and [security](security.md).

## Verification

Core regression tests expose a virtual 5 GiB file whose reader rejects all
requests outside the header and YLSpec frame. They assert the exact total read
volume, plus metadata limits, malformed framing and independence from a corrupt
TOC. Python repeats the virtual-range check; Node, Java and the CLI compare
standalone results to indexed metadata. This tests the distinction between
metadata discovery and full container validation explicitly.
