# Authoring a data plugin

This example produces an unsigned, unencrypted data container with one resource.
It exercises the actual profile and native builder; it does not define an
installer, account system, execution sandbox or host permission vocabulary.

## Directory and resource

Create a working directory containing these three files. Use LF line endings
when comparing byte-for-byte results across platforms.

`catalog.json`:

```json
{"name":"Example catalog","total":42}
```

`plugin.ylspec`:

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

`resources.json`:

```json
{"resources":[{"path":"catalog.json","file":"catalog.json","media":"application/json","compression":0,"extraction":0,"alignment":8}]}
```

The plan's `file` is an author filesystem path, resolved relative to the plan.
Its `path` is the portable resource name stored inside the container. Resource
names use portable ASCII and reject traversal, absolute paths, case aliases,
Windows device names and file/directory collisions. These rules differ from
filesystem paths, where supported SDKs accept valid Unicode.

## Build and verify

Use the actual source-built CLI path from your machine:

```sh
ylplugin create --manifest plugin.ylspec --plan resources.json --output catalog.ylplugin
ylplugin inspect catalog.ylplugin --json
ylplugin validate catalog.ylplugin
```

All output names must be new. Repeating the command with an existing destination
fails; choose another output or remove only your own disposable example output.
The writer validates staged bytes before publishing them without replacement.

## Describe the host separately

`host.json` for a Linux x86_64 data viewer:

```json
{"id":"xyz.example.viewer","api":"1.0","os":"linux","arch":"x86_64","runtimes":{"data":"1.0"},"permission_formats":["strings"],"memory":67108864}
```

```sh
ylplugin check catalog.ylplugin --host host.json
```

The result should be SUPPORTED for this example. Use the real host/platform in
an application. Metadata cannot grant permissions, declare a runtime installed,
or replace the host's actual API version.

## Extend the package deliberately

| Goal | Change |
|---|---|
| More files | Add plan entries with unique portable paths |
| Another host | Add a distinct target ID and explicit host/API bounds |
| Another executable variant | Add an entrypoint and matching resource; native needs concrete OS/architecture/ABI |
| Host permission | Use a host-defined string, mask or byte declaration and require explicit host grants |
| Compression | Select compression 1; decoded size/ratio limits still apply |
| Encryption | Set a key ID and supply the host key provider; keys stay outside the manifest |
| Authenticity | Sign the completed container, then verify against host-controlled trust |

Required and optional permission declarations are independent. Integer masks are
nonnegative signed-64 values; larger masks use byte arrays. Unknown profile keys
are rejected. Put optional vendor metadata under YLSpec extensions rather than
inventing mandatory target fields.

No encryption randomness is introduced by the example above. With identical
metadata/resource bytes and options, the generated fixture is deterministic.
Encryption intentionally generates fresh nonces; do not expect identical
encrypted bytes from repeated builds.

See [the normative profile](profile.md), [CLI](cli.md), [examples](examples.md)
and [signing/encryption coverage](specification.md#authentication-and-confidentiality).
