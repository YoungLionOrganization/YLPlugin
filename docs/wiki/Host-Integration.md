# Host Integration

The host defines the application contract. A plugin declares what it can use;
it cannot declare those services installed or grant itself authority. Separate
inventory/discovery from active runtime sessions.

## Recommended flow

1. Open with explicit resource limits and inspect inert metadata.
2. Resolve publisher trust, signature policy and content-key availability.
3. Construct host state from real API/runtime versions, capabilities, grants,
   platform and memory/storage budgets.
4. Preflight and inspect selected/candidate reasons.
5. Construct the matching approved adapter and Session.
6. Prepare, activate, start, stop and clean up through the host lifecycle.

The data fixture can be read directly by a language SDK without constructing an
execution adapter. A Session is appropriate when a host actually owns a runtime.

## A C++ data host

```cpp
#include <ylplugin/ylplugin.hpp>

ylp::Package package("examples/artifacts/data.ylplugin");
auto host = ylp::host_from_json({
    {"id", "xyz.example.data-viewer"}, {"api", "1.0"},
    {"os", "linux"}, {"arch", "x86_64"},
    {"runtimes", {{"data", "1.0"}}},
    {"permission_formats", {"strings"}}, {"memory", 67108864}
});
auto report = ylp::preflight(package, host);
if (report.at("status") == "SUPPORTED") {
    ylp::Session session(package, host, ylp::data_adapter());
    session.prepare();
    session.activate();
    session.start();
    session.stop();
    session.cleanup();
}
```

Place this fragment in your C++20 host and link `YLPlugin::ylplugin`. The
platform fields describe an example Linux x86_64 application; substitute actual
host values. Session checks the chosen adapter's own ID/version as well as the
metadata declaration.

## Host state to retain

Retain the plugin ID/version, source identity/digest, selected target/entrypoint,
signature decision, explicit grants, runtime versions and preflight reasons.
Keep the trust store and entitlement/key policy in host-controlled state.
Metadata's public publisher string is not an authenticated account record.

Runtime registration belongs to the host. Adding `wasm: 1.0` to a JSON example
is only truthful if the host actually supplies that runtime. Preflight does not
download engines, install dependencies or grant requested permissions.

## Lifecycle and updates

Package references retain a shared implementation. Sessions/adapters are
thread-confined unless explicitly synchronized by the host. Stop consumers and
release their resources before cleanup. A stopped session is not restartable;
create a new one. Destruction performs stop/cleanup but cannot make unsafe host
teardown ordering safe.

For updates, build a new approved session and retire the previous session through
its lifecycle. The library does not provide a transactional application updater,
dependency database or rollback service. See [Runtime Adapters](Runtime-Adapters.md)
and the [complete host guide](../host-integration.md).
