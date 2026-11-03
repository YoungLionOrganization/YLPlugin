# Native C ABI example

`plugin.c` implements ABI 1 query/start/stop/destroy with no Qt dependency.
`host.cpp` applies preflight and an external approved SHA256 before loading.
The example is intentionally separate from LeoMiniGames' Qt IGamePlugin ABI.

The main CMake test build creates the native module and `ylplugin_native_host`.
`examples/build_examples.py --native-library ...` creates a container declaring
the actual OS/architecture/ABI and prints its library digest. Approval must come
from a host-controlled policy; do not trust a hash simply because metadata says it.

See [native example commands](../../docs/examples.md#native-approval). Preparation
checks/extracts; activation can run library constructors. Native code is not
sandboxed. Windows currently rejects secure extraction before activation.
The cache is a new private session directory, not a shared writable install root.
