# Presentation catalog contract — 0.1.4 development

Status: experimental catalog 1.0 declarations. The native C++ parser and authoring
schema are implemented on the 0.1.4 development branch. The latest GitHub/PyPI/JVM
release is 0.1.3. Automatic detection, locator generation and selective asset I/O
remain [roadmap](../ROADMAP.md) work. See the [architecture](design/presentation-assets.md)
for the intended end-to-end behavior.

## Storage and versioning

The catalog is the value of `extensions."xyz.younglion.ylplugin.presentation"` in
a YLSpec document. It contains descriptors only. File contents remain separate
container resources. Container/profile 1.0 and YLSpec 0.1 framing stay unchanged.
The extension is optional; absence means no presentation catalog was declared.

For version `1.0`, the catalog accepts exactly `version`, `assets` and optional
`primary`. Unknown catalog versions produce an unsupported result when explicitly
parsed. Unrelated optional extensions remain inert. The parser does not register
a runtime, grant permissions or change plugin-profile validation.

## Descriptors

`assets` is an array sorted strictly by ASCII `(role, id)`. IDs are unique, 1..255
ASCII letters, digits, dots, underscores or hyphens. Supported roles are `icon`,
`readme`, `license`, `contributing`, `security`, `code_of_conduct`, `changelog`,
and `notice`. Each descriptor has these exact fields:

| Field | Contract |
|---|---|
| `id`, `role` | Identifier and one supported role |
| `resource` | Portable container path, 1..1024 bytes; native `safe_path` rules apply |
| `media_type`, `format` | Supported pair below |
| `encoding` | Required `utf-8` for documents; absent for icons |
| `offset` | Nonnegative signed-64 integer, divisible by 8; absolute package position |
| `length` | Nonnegative signed-64 integer; exact stored/plaintext byte length |
| `sha256` | Exactly 64 lowercase hexadecimal characters |

Inline document/image bodies and unknown fields are rejected. Icons accept
`png`/`image/png`, `webp`/`image/webp`, or `jpeg`/`image/jpeg`, with nonzero length.
Other roles accept `markdown`/`text/markdown` or `text`/`text/plain`, encoded as
UTF-8. Empty documents use the SHA-256 of empty bytes, not an arbitrary checksum.

`primary` maps a supported role to an existing ID of that same role. Several
license documents can coexist; the primary selection is a UI preference, not
legal interpretation. The role does not imply an SPDX license expression.

Repeated physical resources are allowed only as exact aliases: package path,
range, hash, media type, format and encoding must agree. Distinct resources must
not overlap. Case-aliased paths and file/directory prefix collisions are rejected.
Zero-length ranges do not consume bytes or create an overlap by themselves.

The native default limits are 64 entries, 1 MiB per document, 2 MiB per icon and
16 MiB aggregate declared bytes. Aliases still count toward declaration budgets.
Custom `PresentationLimits` can lower or raise these budgets. Source metadata
must be parsed under its own YLSpec/document limits before catalog validation.

## Native C++ API

```cpp
#include <ylplugin/presentation.hpp>

// document is a caller-owned, already parsed yl_document.
auto catalog = ylp::parse_presentation_catalog(document);
if (catalog) {
    for (const auto& asset : catalog->assets) {
        // asset.offset and asset.length are exact uint64_t values.
        // asset.hash owns the 32 bytes represented by sha256.
    }
}
```

The result owns its strings and buffers and survives destruction of the source
document. A missing namespace returns `std::nullopt`. Unsupported versions throw
`Code::unsupported`; malformed declarations throw `Code::invalid_metadata`;
exceeded budgets throw `Code::resource_limit`.

This function performs **no file I/O**. It does not check a container's physical
size/payload bounds, inspect the TOC, compare resource bytes to hashes, validate
the plugin profile or authenticate a publisher. A valid catalog declaration is
not a valid/verified package. Future asset reads must add all necessary source-
range and content checks before returning bytes. No discovery handle or asset
export command is available yet, and bindings do not expose this new helper yet.

## Authoring schema and vectors

[schemas/presentation.schema.json](../schemas/presentation.schema.json) validates
the catalog object, not the whole YLSpec envelope. The existing plugin schema
continues to define the envelope/profile; this schema does not change profile 1.0.
Schema defaults match native budgets. Native validation is authoritative for
unique IDs, primary references, ordering, portable-path details, aliases,
overlaps and aggregate limits.

JSON Schema treats a JSON number such as `4096.0` as an integer mathematically.
YLSpec retains TOML's integer/float type distinction, so the native validator
rejects a floating offset even when its numeric value is integral. Producers
must supply actual integers and preserve their exact values across languages.

Shared [vectors](../tests/presentation/catalog-cases.json) record structural
schema expectations separately from native error codes. The native test builds
YLSpec values through YLSpec's own API; it does not add a second container parser.
Positive declaration fixtures do not claim to contain matching file bytes.
