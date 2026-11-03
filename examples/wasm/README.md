# Host-supplied WebAssembly example

`add.wat` documents the tiny `add(i32,i32)` module; `add.wasm` is the actual binary
carried by `../artifacts/wasm.ylplugin`. The Node host preflights its own registered
WASM runtime, reads verified bytes, then explicitly instantiates the module.
Expected output: 42.

This illustrates the runtime boundary without pretending that the core includes
a built-in WASM engine. Preflight itself executes no plugin code. Build the Node
addon and run the [documented command](../../docs/examples.md#host-supplied-webassembly).
An unmodified host that has not registered this runtime reports NEEDS_RUNTIME.
