# CLI Cookbook

**0.1.3 additions:** [direct metadata-only reads](Metadata-Only-Reads.md)
request only the 128-byte header and YLSpec frame. TOC, payload and signature
regions are not read. Discovery returns unverified metadata; validation and host
preflight remain separate. Python/npm now bundle the native CLI alongside the
core; Java/Kotlin supply a Java CLI entry point. See
[core/CLI installation and PATH behavior](../installation-cli.md).
These additions are available from 0.1.3.

The CLI inspects and builds containers. None of its commands automatically
starts plugin code, installs a runtime, grants host permissions or publishes to
registries. Success output is JSON on stdout; error JSON goes to stderr.

## Identify and inspect

```sh
ylplugin --version
ylplugin inspect data.ylplugin --json
ylplugin list data.ylplugin --json
ylplugin validate data.ylplugin
```

Inspection reports public TOML and resource metadata. Validation checks the
profile and resource integrity; it does not authenticate an unsigned package or
turn UNVERIFIED into trusted publisher authority.

## Build from explicit inputs

```sh
ylplugin create --manifest plugin.ylspec --plan resources.json --output new.ylplugin
ylplugin create --manifest plugin.ylspec --root resources --output root-build.ylplugin
```

A plan records portable paths, filesystem files, media types and optional
compression/extraction/alignment/variant/key settings. Plan files are resolved
relative to the plan directory. Root builds reject symlinks and nonregular files.
Output packages must not exist. `--compression store|zstd` applies to root builds;
RCC defaults to store to avoid double compression.

## Evaluate a host

```sh
ylplugin check data.ylplugin --host host.json
ylplugin check data.ylplugin --host host.json --target viewer
```

Host descriptions contain actual host/API/platform/runtime data, explicit grants
and budgets. `--os`, `--arch` and `--abi` are available for explicit evaluation.
Check does not create runtime support or install a missing dependency. Inspect
JSON status/reasons when it returns a non-SUPPORTED result.

## Extract or convert

```sh
ylplugin extract data.ylplugin --output NEW_PRIVATE_DIRECTORY --resource catalog.json
ylplugin convert legacy.rcc --manifest plugin.ylspec --output converted.ylplugin
```

Omitting `--resource` extracts all resources. Direct-required records prohibit
extraction. POSIX extraction requires a suitably private parent and a new
destination; symlink/traversal protections are deliberate. Secure Windows
extraction is not implemented. Convert preserves RCC bytes and embeds explicit
metadata; it does not rewrite the file or create a QML application host.

## Exit status

| Exit | Meaning |
|---:|---|
| 0 | Success / preflight SUPPORTED |
| 3 | Invalid input, arguments, metadata, limits or denied extraction policy |
| 4 | Preflight not SUPPORTED; inspect report |
| 5 | Hash, GCM authentication or signature failure |
| 6 | Missing content key/license |
| 7 | I/O or non-replacing output publication failure |
| 8 | Cooperative cancellation |
| 9 | Unsupported operation/codec/command |

These CLI exits differ from native SDK status values. For example native code 6
means integrity, while CLI exit 6 means missing content key. Use the right table
for the interface you are calling.

SIGINT/SIGTERM request cooperative cancellation and staging cleanup. Forced
termination/power loss cannot run destructors. The CLI does not expose arbitrary
limit overrides; a host with different capacity policies uses the SDK.

See [the reference](../cli.md), [authoring](Authoring-Plugins.md) and
[Signing and Encryption](Signing-and-Encryption.md).
