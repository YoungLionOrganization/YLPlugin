# Host integration

A host owns compatibility, trust, permissions, budgets and runtime services.
YLPlugin supplies container validation and reports; applications decide what
those reports authorize. Loading the SDK itself is separate from loading a
plugin's native code or mounting its RCC content.

## A two-stage integration

1. Discover/inspect the container and select host policy without running it.
2. Verify as required, preflight using real host state, and construct an approved
   runtime/session only when the result allows it.

Keep plugin inventory records separate from active sessions. Record plugin
ID/version, selected target/entrypoint, signature decision, host API/runtime
versions, permission grants and failure reasons in your own application state.
Do not log decrypted resources or provider secrets merely to diagnose preflight.

## C++ data session

This Linux x86_64 example uses the checked-in data fixture:

```cpp
#include <ylplugin/ylplugin.hpp>
#include <iostream>

int main() {
    ylp::Package package("examples/artifacts/data.ylplugin");
    auto host = ylp::host_from_json({
        {"id", "xyz.example.data-viewer"}, {"api", "1.0"},
        {"os", "linux"}, {"arch", "x86_64"},
        {"runtimes", {{"data", "1.0"}}},
        {"permission_formats", {"strings"}},
        {"memory", 64 * 1024 * 1024}
    });
    auto report = ylp::preflight(package, host);
    std::cout << report.dump(2) << '\n';
    if (report.at("status") != "SUPPORTED") return 1;
    ylp::Session session(package, host, ylp::data_adapter());
    session.prepare();
    session.activate();
    session.start();
    session.stop();
    session.cleanup();
}
```

Build with C++20 and link `YLPlugin::ylplugin`. Replace the platform fields with
the real host configuration. The Session repeats required checks and verifies
that the chosen adapter's ID/version actually matches the selected entrypoint.
Data sessions decode/read data; they do not execute a dataset as code.

## Preflight decisions

| Status | Host action |
|---|---|
| SUPPORTED | Continue with the matching approved adapter and lifecycle |
| UNSUPPORTED / UNSUPPORTED_PROFILE | Reject this target or choose an allowed fallback |
| NEEDS_RUNTIME | Register/install a runtime through a separate host-controlled process |
| NEEDS_PERMISSION | Obtain an explicit host-scoped grant; never grant automatically |
| NEEDS_LICENSE | Resolve the content key/entitlement through the host |
| NEEDS_EXTRACTION | Decide whether a private extraction strategy is permitted |
| RESOURCE_LIMIT / UNKNOWN | Supply adequate known budgets or reject |
| INVALID_SIGNATURE / INVALID_CONTAINER | Reject and report the validation/trust failure |

Targets are ranked by exact host match before explicit generic opt-in, then
priority and lexical target ID. A target with fallback false stops fallback
selection when it cannot be used. Inspect candidate reasons instead of reducing
all failures to a generic incompatible message.

## Separate trust and content keys

`host.trust` supplies publisher public keys and can reject revoked/unknown IDs.
`host.keys` supplies 32-byte content keys and may enforce account/entitlement
policy. A content key is not a publisher identity. If the host requires signed
packages, set `require_signature` and supply trust. Public `publisher` metadata
is informational until your policy authenticates and authorizes the publisher.

## Runtime-specific authority

* Native: explicitly opt into native execution/extraction and approve the digest
  against a host-controlled reviewed allowlist. Native code is not sandboxed.
* RCC: review the content before Qt parses it. Destroy engines/QML objects and
  resource users before cleanup; the adapter only mounts bytes.
* WASM: the example supplies its own host WebAssembly runtime after preflight;
  the core does not implement an automatic WASM adapter.
* Python/Java/JS SDKs: language bindings are host interfaces, not plugin execution runtimes.

## Lifetimes, cancellation and updates

Package references live while a shared package implementation remains alive.
Sessions/adapters are host-thread confined unless the host synchronizes them.
Do not close a handle from an active callback or wait in a callback for another
thread that requires the same handle lock. Cancellation is cooperative; custom
readers must cooperate with cancellation during their own blocking I/O.

On application shutdown stop sessions, destroy runtime consumers, clean adapters,
then release packages. Updating a plugin should create a new approved session
and retire the previous session through its lifecycle. YLPlugin does not supply
an installer, dependency solver, live code patcher or rollback database.

See [runtimes](runtime.md), [API ownership](api.md), [permissions](profile.md#permissions),
[security](security.md) and [troubleshooting](troubleshooting.md).
