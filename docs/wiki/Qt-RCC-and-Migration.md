# Qt RCC and Migration

Qt is optional. The container, core CLI and language SDKs do not require it.
The `qt.rcc` adapter registers reviewed RCC bytes in memory; it does not create
a QQmlEngine, provide application services or grant QML access to the host.

## Convert without rewriting content

```sh
ylplugin convert legacy.rcc --manifest plugin.ylspec --output converted.ylplugin
ylplugin inspect converted.ylplugin --json
ylplugin validate converted.ylplugin
```

The explicit manifest declares a resource/entrypoint for the RCC. The conversion
preserves RCC payload bytes; the checked-in demo is a real RCC/QML example.
Container validation does not validate every internal Qt RCC parser invariant.

## Build and host integration

Enable `YLPLUGIN_QT=ON` with QtCore 6.4+ available. Consumers use the optional Qt
library and `ylplugin/qt.hpp`. `rcc_adapter` takes a mount root and an explicit
approval callback. The host reviews the bytes and publisher policy before
allowing Qt to parse them. A signature alone is not sufficient RCC parser approval.

Register the real `qt.rcc` runtime/version in host state and follow the Session
lifecycle. The host creates engines/objects, supplies services and controls when
they are destroyed. Existing QML or game API contracts must still be honored.

## Teardown ordering

Destroy QML objects, engines, callbacks, QFiles and QResources using the mounted
data before session cleanup. The adapter retains its buffer while mounted. If
Qt reports outstanding references during unregister, bytes are retained until
process exit rather than force-freed. Repeated busy cleanup is not a bounded
cache and must be controlled by the host's lifetime design.

Do not treat memory pressure as permission to free a buffer still owned by Qt
consumers. On mobile, host lifecycle services must stop/pause activity and tear
down consumers before releasing mounted resources.

## Migration boundaries

YLPlugin is independent of LeoMiniGames and does not change its existing loaders.
The native C runtime ABI here is not the historical Qt IGamePlugin ABI. A host
integration needs actual retained fixture/application tests; this documentation
does not prove that an unmodified application accepts a new container.

YLPkg is a separate future installer boundary. RCC conversion is not dependency
installation, account migration or an updater. See the
[migration guide](../migration-rcc.md), [RCC example](../../examples/rcc/README.md)
and [runtime teardown contract](../runtime.md).
