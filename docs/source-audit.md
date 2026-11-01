# Source audit — 2026-11-01

Only YLPlugin is modified by this project. Existing isolated checkouts were read.
GitHub default-branch commit checks matched these local revisions:

* YLSpec `74ef1470105566b812c8a712ce56848f98c277cb` (0.1.2).
* LeoMiniGames `3e85f539d66111a51c59fad5c8c4390527489afd` (0.7.3 sources).

YLSpec: read `docs/specification.md`, `embedding.md`, `profiles.md`, `c-api.md`,
`python-api.md`, `compatibility.md`, `SECURITY.md`, `include/ylspec.h` and CMake.
Use its actual `yl_parse`, `yl_validate`, `yl_profile`, `yl_registry`, `yl_pack`,
`yl_read_at` and typed value API. The core is C11; the 32-byte frame header is
not the whole document. Integers are signed 64-bit; `yl_dump` emits decimal.
Unknown profiles can be inspected, never treated as validated. No core change.

LeoMiniGames: read `RccPackageInspector.{h,cpp}`, `PluginManager.cpp`,
`GameResources.cpp`, `ExternalGameRuntime.cpp`, `ModManager.cpp`,
`sdk/IGamePlugin.h`, SDK manifest/compatibility/native docs and TRUSTED_NATIVE.
Canonical RCC has root manifest/Main.qml and mounts under `/mods/<id>`; legacy
RCC already contains `/mods/<id>` and must not expose another namespace.
Inspector validates IDs, paths, manifest limit 1 MiB, resource limit 10000 and
unambiguous legacy namespaces. Packaging MUST preserve RCC bytes and manifest.
Missing `api_version` negotiates v0.5/v0.6; API 0.7 adds explicit hard capability
requirements and compatibility facades. Do not invent an API 0.7 native ABI:
native IID is `xyz.younglion.leominigames.IGamePlugin/1.0`, API integer 1.
Native loading requires reviewed + publisher_verified + exact SHA256 + Qt major
and API match from the local trust snapshot. Metadata booleans are not trust.
Android/iOS omit external Level 3 loading. QML engine/object teardown precedes
resource release. YLPlugin must be an additive host integration, not a replacement.

Related local projects: JSON-API-Forge README describes disabled-until-reviewed
native plugins and digest-checked ZIP imports; its format is not a YLPlugin
standard. YoungLionPythonLibrary and terminal library describe general native
utilities/UI, not a reusable plugin container. None supplies a known YLPkg
implementation to copy; YLPkg remains a separate future consumer.

Qt 6 documentation and installed `qresource.h` confirm the memory overload
`registerResource(const uchar *, const QString &)`. The bytes must outlive every
QFile referencing them, including after unregister. Unregister returns false
when references remain. The optional adapter consequently retains such buffers
until process exit instead of risking use-after-free; hosts should tear down
engines/files first. Qt RCC parsing itself requires host-reviewed bytes.
