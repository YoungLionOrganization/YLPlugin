# CLI reference

## Metadata-only command and package installation (0.1.3 development)

`ylplugin metadata FILE` emits canonical TOML and reads only the container header
and YLSpec frame. `--json` emits `scope: "metadata-only"`,
`integrity_status: "UNVERIFIED"`, and the TOML string in `metadata`. It does not
construct a Package or read the TOC/payload/signature. This is a new command;
other commands retain their existing behavior. See [metadata-only reads](metadata-only.md).

Python wheels now register the native CLI with pip, npm supplies a `bin` entry,
and Java/Kotlin bundle a Java CLI entry point. See [installation](installation-cli.md).


Commands below are implemented in the indicated source version. None starts plugin code. Use `--json` for
compact machine-readable JSON; default output is indented JSON except the metadata command, which emits TOML. Metadata is
returned as public TOML inside the inspect report. No command publishes, installs
components, grants permissions or contacts a network service.

For a step-by-step workflow see the [English Wiki cookbook](wiki/CLI-Cookbook.md)
and [plugin authoring guide](plugin-authoring.md). Native SDK error statuses
and these CLI process exit codes are different interfaces.

```sh
ylplugin --version
ylplugin create --manifest plugin.ylspec --root resources --output game.ylplugin
ylplugin create --manifest plugin.ylspec --plan resources.json --output game.ylplugin
ylplugin inspect game.ylplugin --json
ylplugin list game.ylplugin --json
ylplugin validate game.ylplugin
ylplugin check game.ylplugin --host host.json --target my-target --os linux --arch x86_64 --abi my-abi
ylplugin extract game.ylplugin --output NEW_DIRECTORY --resource assets/data.bin
ylplugin convert legacy.rcc --manifest plugin.ylspec --output game.ylplugin
ylplugin sign game.ylplugin --output signed.ylplugin --key-id publisher-v1 --private-key private.pem
ylplugin verify signed.ylplugin --key-id publisher-v1 --public-key public.pem
ylplugin encrypt game.ylplugin --output encrypted.ylplugin --key-id content-v1 --key-file content.key
```

All outputs MUST be new; existing destinations are never replaced. Extraction's
parent must be privately writable by the current user (see security.md). Omit
`--resource` to extract all resources; direct-required resources block extraction.
`--compression store|zstd` applies to root builds; `.rcc` defaults to store even
when zstd is requested. Root builds reject symlinks and non-regular files. Plan
file paths are relative to the plan's directory and are explicit author inputs:

```json
{"resources":[{"path":"game.rcc","file":"assets/game.rcc",
  "media":"application/x-qt-resource","compression":0,
  "extraction":0,"alignment":8}]}
```

Optional record fields: `variant` (entrypoint ID), `key_id` (encrypted resource),
`compression` 0/1, `extraction` 0=auto/1=direct-required/2=extract-required,
`alignment` power-of-two 8..4096. Create supports one CLI key with exact ID through
`--key-id/--key-file`; the SDK callback supports multiple keys.

The key file contains exactly 32 RAW bytes; it is not a hex string. Generate it
with `openssl rand -out content.key 32` in private storage. Never put it in TOML,
source control or public assets. Private signing keys are unencrypted Ed25519 PEM
for this CLI; encrypted PEM input fails without prompting. Protect the file or
use the SDK's in-memory key interface. Verify uses Ed25519 public PEM plus an
explicit key ID; that pair is an explicit trust decision by the caller.

Validate/check/extract accept `--key-id/--key-file`; check also accepts verify's
`--public-key` option and the same exact key ID. The CLI supports a single key
provider and a single trusted signer per invocation. If content and signer IDs
differ, use SDK providers or verify and validate as separate commands. Check's
host JSON cannot embed keys. CLI encrypt currently accepts plaintext input only,
encrypts all records, and rebuilds through the native writer. Signed input requires
explicit `--drop-signature`; sign the new encrypted output again. Rekeying an
encrypted package uses the SDK builder/providers and is not a pretend CLI feature.

| Exit | Meaning |
|---:|---|
| 0 | Operation succeeded / preflight SUPPORTED |
| 3 | Invalid input, metadata, limits, arguments or denied extraction policy |
| 4 | Preflight not SUPPORTED (inspect JSON status/reasons) |
| 5 | Hash, authentication tag or digital signature failure |
| 6 | Missing license/encryption key |
| 7 | I/O or non-replacing publication failure |
| 8 | Cooperative cancellation |
| 9 | Unsupported operation/codec/command |

Error JSON is written to stderr, success JSON to stdout. Do not infer publisher
trust from `validate`: it reports signature status UNVERIFIED/UNSIGNED. `verify`
is separate. Unknown/incompatible files return a nonzero exit without loading
anything. Per-record decoding is bounded rather than whole-container buffering;
single records above the configured 64 MiB default require an SDK limit override.
CLI does not expose limit overrides; custom high-capacity hosts use the SDK.
SIGINT/SIGTERM request cooperative cancellation of long operations, with exit 8
and normal staging cleanup. SIGKILL/power loss cannot run cleanup; see security.md
for abandoned private staging directories.

## Authoring examples and registry scope

The same CLI can package JSON datasets, native modules and WASM blobs as well as
RCC. `examples/build_examples.py` writes explicit plans and validates generated
containers. The CLI never turns a data entry into executable code automatically.
Registry publication is deliberately outside CLI commands; use the manual
[verified publishing workflow](publishing.md), not a plugin-supplied instruction.
