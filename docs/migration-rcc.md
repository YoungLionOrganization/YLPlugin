# RCC and LeoMiniGames migration

This is an additive integration plan, not a change to LeoMiniGames. Existing RCC
loaders, trust snapshots, SDKs and v0.5/v0.6/v0.7 compatibility are unchanged.
`xyz.younglion.leominigames` is the proposed YLPlugin host target ID; it is not an
already exported LeoMiniGames YLPlugin API.

```sh
ylplugin convert legacy-game.rcc --manifest plugin.ylspec --output game.ylplugin
ylplugin inspect game.ylplugin --json
ylplugin validate game.ylplugin
```

Conversion copies the RCC byte-for-byte into a store record; it never recompiles,
renames internal resources, modifies `manifest.json` or rewrites QML. The explicit
outer profile manifest is required. If all entrypoints reference the same resource,
convert uses that path; otherwise `--resource` selects the path, and all remaining
entrypoint references must still exist (use create/plan for multi-resource builds).
Conversion checks RCC magic/minimum header only, not the internal Qt tree or LMG
semantic compatibility. `validate` verifies the container/resource digest, not
the safety or correctness of RCC contents. The host must retain its inspector.

| Existing content | Additive host treatment |
|---|---|
| Plain `.rcc` | Continue current `RccPackageInspector`/ModManager path |
| Wrapped root-layout RCC | Mount under `/mods/<id>` after existing checks |
| Wrapped legacy `/mods/<id>` RCC | Preserve namespace; do not prefix it twice |
| No inner `api_version` (v0.5/v0.6) | Retain the legacy negotiation/facades; do not upgrade by outer metadata |
| Explicit v0.7 inner API | Run the existing min/requested API + hard capability negotiation |
| Native Qt `IGamePlugin` | Keep reviewed/publisher_verified/hash/API/Qt checks from trusted-native.json |

A proposed host integration sequence:

1. Detect YLPlugin by authoritative file selection + validated magic, open without
   code execution, apply container limits and plugin profile validation.
2. Verify signatures according to host policy; ask the host key provider for any
   encrypted selected resource. Treat publisher as untrusted until host review.
3. Preflight with the real host API, capabilities, OS/CPU/ABI, supported runtimes,
   exact permission registries, explicit grants, resource budgets and signature
   policy. Preserve target/entrypoint/runtime/strategy/reason diagnostics.
4. For RCC, validate the selected decoded bytes using the host's independent RCC
   inspection/trust boundary. Keep current manifest size/resource count/ID/entry/
   namespace validation. The current LMG inspector takes a filename: until a
   reviewed bounded memory inspector exists, use its controlled extracted path,
   or approve only known reviewed RCC hashes for the memory adapter. Do not feed
   arbitrary new RCC bytes to the lengthless Qt API merely because a TOC hash matches.
5. Supply the correct mount root to `rcc_adapter`. For canonical resources this is
   `/mods/<id>`; legacy internal prefixes require the existing root convention.
   The host owns namespace collision detection. The adapter does not parse inner
   LMG IDs or choose resource roots automatically.
6. Instantiate the existing ExternalGameRuntime/QML service facades with the same
   `qrc:/mods/<id>/<entry>` URL and inner manifest. Preserve v0.5/v0.6 facades and
   v0.7 hard requirements. Outer permission metadata never grants host internals.
7. Stop background callbacks/game objects, tear down QML engine/files, then cleanup
   the adapter and release memory. Busy unregister retains buffers until exit.
   In the inspected ExternalGameRuntime, `unload()` retires objects through a
   zero-delay QTimer callback; its return does not mean engine destruction has
   finished. Keep the mount/session alive until the retired engine and its files
   have actually been destroyed, rather than unmounting immediately after unload.

The generic native adapter implements YLPlugin's new C ABI, not Qt IGamePlugin.
LeoMiniGames should register a separate `lmg.native` adapter with explicit API 1,
Qt major/compiler ABI, trusted directory and reviewed digest/publisher policy;
the existing loader can remain authoritative. A valid YLPlugin signature is an
additional evidence source, not an override of the trusted-native snapshot.
Do not dynamically activate external native code on Android/iOS.

The sample under `examples/artifacts` contains a real root-layout RCC built from
this project's own QML and manifest. It is a packaging/memory-mount fixture, not
a claim that the current unchanged LMG executable already opens `.ylplugin`.
The memory RCC runtime mounts data; starting the embedded QML remains a host action.
