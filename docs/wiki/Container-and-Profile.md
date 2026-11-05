# Container and Profile

**0.1.3 development additions:** [direct metadata-only reads](Metadata-Only-Reads.md)
request only the 128-byte header and YLSpec frame. TOC, payload and signature
regions are not read. Discovery returns unverified metadata; validation and host
preflight remain separate. Python/npm now bundle the native CLI alongside the
core; Java/Kotlin supply a Java CLI entry point. See
[core/CLI installation and PATH behavior](../installation-cli.md).
These additions are not available in the published 0.1.2 packages.

## Independent versions

| Layer | Purpose |
|---|---|
| SDK 0.1.x | Library/API implementation and package compatibility |
| Container 1.0 | Byte layout, offsets, record and algorithm identifiers |
| YLSpec 0.1 frame | Embedded metadata framing |
| Plugin profile 1.0 | Entrypoints, host targets and permission declarations |
| C ABI version 1 | Foreign language structure/call compatibility |
| Runtime and host API versions | Actual activation compatibility |

An application's own plugin version in `[data]` is not the SDK version. Patching
the SDK does not silently redefine container algorithms or host permission bits.

## Byte layout

A `.ylplugin` file has a 128-byte little-endian header, one exact embedded YLSpec
frame, an aligned table of contents, aligned payload records and an optional
128-byte signature block. The reader checks physical/declarative lengths,
overflow, ordering, padding and mandatory flags before using resource offsets.

TOC paths are sorted portable ASCII names. Duplicate/case-aliased names,
traversal and file/directory prefix collisions are invalid. Each record carries
stored/plaintext sizes, codec/encryption/extraction options, alignment, SHA256,
and optional nonce/tag/key/variant information. Container resource names are
not arbitrary operating-system filesystem paths.

Store records have equal stored/plaintext sizes. Zstd records contain exactly
one known-size frame without a dictionary or trailing frames. AES-GCM encrypts
the stored/compressed record; authentication precedes decompression and return
of bytes. Metadata stays public.

## Profile model

`data` contains plugin ID/version, nonempty entrypoint and target arrays, and
optional informational publisher text. Each target references an existing
entrypoint, and each entrypoint references a TOC resource. Nonempty record
variants bind a resource to an entrypoint ID.

An entrypoint declares runtime ID, inclusive numeric runtime bounds and optional
OS/architecture/ABI constraints. Native entries require concrete platform and
ABI values. A target adds host ID/API range, ranking/fallback behavior,
capabilities, permission declarations and resource budgets.

Numeric versions have one to three components with at most nine digits per
component. Missing components compare as zero. Prerelease labels, caret/tilde
ranges and implicit wildcard version parsing are not supported.

Unknown profile keys and required features fail validation. Optional vendor
metadata belongs in namespaced YLSpec extensions. An unfamiliar profile may be
inspected inertly but is not approved for validation/activation.

## Read the normative contract

This Wiki summarizes the model; exact offsets and MUST requirements are in the
[specification](../specification.md) and [profile](../profile.md). Use the native
builder and conformance vectors to avoid duplicating parser/crypto logic in an
application. [Authoring Plugins](Authoring-Plugins.md) provides a complete example.
