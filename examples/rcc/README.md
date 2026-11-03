# RCC/QML example

This project's own `Main.qml` and `manifest.json` produce a real root-layout RCC.
`../artifacts/demo.ylplugin` carries that RCC unchanged. The optional QtCore
adapter supports memory registration; the host owns QML engine creation and
service/publisher policy. Existing LeoMiniGames RCC loaders are unchanged.

`examples/build.sh` builds into a new `build/rcc-example` directory by default,
preserving checked-in fixtures. Pass another new directory for a second run.
Set RCC to the Qt resource compiler and YLPLUGIN_CLI to the built CLI if needed.

Destroy QML objects/engines/QFiles before unmount. Busy unregister retains bytes
instead of freeing memory still referenced by Qt. RCC approval must include the
host's independent inspection/review, not merely a matching TOC hash. Read the
[migration guide](../../docs/migration-rcc.md) and [runtime contract](../../docs/runtime.md).
