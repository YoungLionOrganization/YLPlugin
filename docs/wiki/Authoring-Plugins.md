# Authoring Plugins

Treat authoring as three related inputs: a public YLSpec manifest, resource
files, and a plan mapping filesystem inputs to portable container paths. An
author's entitlement or publisher account is not stored as a secret in metadata.

## Minimal data manifest

```toml
[_ylspec]
version = "0.1"
profile = "xyz.younglion.plugin"
profile_version = "1.0"
[data]
id = "xyz.example.catalog"
version = "1.0.0"
[[data.entrypoints]]
id = "catalog"
runtime = "data"
runtime_min = "1.0"
runtime_max = "1.0"
resource = "catalog.json"
[[data.targets]]
id = "viewer"
host = "xyz.example.viewer"
api_min = "1.0"
api_max = "1.0"
entrypoint = "catalog"
required_permissions = []
```

Create `catalog.json` with `{"name":"Example catalog","total":42}` and
`resources.json` beside it:

```json
{"resources":[{"path":"catalog.json","file":"catalog.json","media":"application/json","compression":0,"extraction":0,"alignment":8}]}
```

```sh
ylplugin create --manifest plugin.ylspec --plan resources.json --output catalog.ylplugin
ylplugin inspect catalog.ylplugin --json
ylplugin validate catalog.ylplugin
```

`file` is relative to the plan's directory; `path` is the name visible to SDK
read calls. The output must not already exist. Use a new work/output directory
for repeated experiments rather than assuming replacement is supported.

## Multiple hosts and variants

Add independently unique target IDs for different hosts. Give each explicit
numeric API bounds and an entrypoint reference. A generic `host = "*"` target
is considered only when the real host explicitly opts in. Priority and fallback
control selection; neither overrides platform, permission or budget checks.

An entrypoint variant may reference another resource with a different runtime
or platform. For native libraries, supply concrete OS/architecture/ABI metadata
and package the actual platform library. Do not rename a foreign binary or
invent a platform declaration solely to satisfy preflight.

## Compression, encryption and reproducibility

Compression 0 is store; 1 is Zstd. Extraction is 0 automatic, 1 direct-required,
or 2 extract-required. Alignment is a power of two from 8 to 4096. The SDK
enforces decoded size/aggregate/ratio policies even for author-created packages.

A key ID associates encryption with a host-provided 32-byte key; it is not the
key itself. Encryption creates fresh random nonces. Unsigned, unencrypted
outputs with identical metadata/resource bytes and options are deterministic.
Use LF source fixtures when comparing bytes across platforms.

For signed/encrypted workflows read [Signing and Encryption](Signing-and-Encryption.md).
For the complete copyable authoring/host example see
[the authoring guide](../plugin-authoring.md).
