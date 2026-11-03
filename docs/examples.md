# Runnable examples

The fixtures are authored by this project. Their metadata is untrusted until the
host applies its policy; examples deliberately use no production credentials.

| Container | Source | Execution model |
|---|---|---|
| `examples/artifacts/data.ylplugin` | `examples/data/catalog.json` | Read-only data, multiple hosts, no Qt |
| `examples/artifacts/wasm.ylplugin` | `examples/wasm/add.wat` / `add.wasm` | Node host supplies WebAssembly explicitly |
| `examples/artifacts/demo.ylplugin` | RCC/QML sample | Optional reviewed RCC memory mount |
| Generated `native.ylplugin` | `examples/native/plugin.c` | Approved platform-specific library |

Build the core with tests enabled, then create additional containers in a new
directory. Paths below assume a single-config Linux build:

```sh
python examples/build_examples.py build/generated-examples --cli build/ylplugin \
  --native-library build/libylplugin_native_example.so
build/ylplugin validate build/generated-examples/data.ylplugin
```

Use the appropriate DLL/dylib/module path from your build on other platforms.
Generated plan files document every input. The builder refuses existing output
directories/packages and validates each result. Data/WASM fixture bytes are
checked for reproducibility; the native container declares the actual OS/CPU/ABI.

## Data and multiple hosts

The catalog has total 42. `viewer` requests no permissions, `analysis` requests
the maximum signed-64 mask in a host-defined registry, and `generic` requires
explicit host opt-in. No bit receives an automatic meaning.

```sh
build/ylplugin check examples/artifacts/data.ylplugin --host examples/data/host.json
PYTHONPATH=bindings/python python examples/data/read.py build/libylplugin.so examples/artifacts/data.ylplugin
YLPLUGIN_NODE_PATH="$PWD/build/ylplugin.node" node examples/data/read.mjs examples/artifacts/data.ylplugin
```

See the Java/Kotlin API guides for equivalent try-with-resources/use examples.

## Host-supplied WebAssembly

```sh
YLPLUGIN_NODE_PATH="$PWD/build/ylplugin.node" node examples/wasm/host.mjs examples/artifacts/wasm.ylplugin
```

Output is 42. Preflight checks the host's registered `wasm` 1.0 runtime without
executing code. The host then reads hash-verified bytes and explicitly calls
WebAssembly.instantiate. This does not add a built-in WASM adapter to the SDK.

## Native approval

The generator prints the digest of the locally built library. Pass an externally
approved digest to the sample host; never approve by comparing a TOC hash with
the plugin's own identical claim.

```sh
build/ylplugin_native_host build/generated-examples/native.ylplugin \
  build/new-private-session-cache APPROVED_SHA256_FROM_HOST_POLICY
```

The host preflights, approves, extracts, loads, starts, stops, unloads and removes
the private cache. Wrong digests deny preparation. Windows currently reports
unsupported extraction before loading. The example uses YLPlugin's C native ABI,
not LeoMiniGames' Qt IGamePlugin ABI, and is not a native-code sandbox.

## Encrypted and signed data

```sh
YLPLUGIN_CLI="$PWD/build/ylplugin" sh examples/security.sh build/new-private-security-example
```

The script creates fresh raw AES and Ed25519 PEM keys under a private directory,
encrypts the data fixture, signs, verifies and validates it. An optional second
argument selects another input container. Keep keys out of source control and
public assets. Verification uses the publisher key; decryption uses the separate
content key. Python/JS/Java integration tests also exercise wrong keys, missing
keys, revoked signers, callback lifetime and exact permission integers.
