# Kotlin SDK

## Metadata discovery and CLI (0.1.3 development)

`xyz.younglion.ylplugin.kotlin.readMetadata(path: Path)` returns canonical TOML
through Java/JNI. `YLPlugin.readMetadata(path, limits)` accepts explicit limits.
Both read only the header and YLSpec frame. See [metadata-only reads](metadata-only.md).
Use `xyz.younglion.ylplugin.Cli.run(...)` for the bundled native CLI; Gradle resolves
the core transitively without changing the system PATH. See [installation](installation-cli.md).


Coordinates: `xyz.younglion:ylplugin-kotlin`, version 0.1.2, published on Maven Central. The
JVM target is 11 and the wrapper depends on Java/JNI. Kotlin adds `use` helpers,
not another native interface or container parser.

```kotlin
import java.nio.file.Path
import xyz.younglion.ylplugin.kotlin.openPlugin
import xyz.younglion.ylplugin.kotlin.readText

openPlugin(Path.of("data.ylplugin")) { plugin ->
    plugin.validate()
    println(plugin.readText("catalog.json"))
}
```

`openPlugin` closes even if the callback throws. `readText` decodes UTF-8; use
Java `Plugin.read` for binary data. `isSupported(host)` checks native preflight's
SUPPORTED result and does not grant permissions or start a runtime. Java methods
and callbacks remain available, including exact Long masks, signing, encryption
providers and selected extraction.

Gradle creates sources/documentation JARs and a staging Maven repository.
Signing/publication is a separate manual workflow, not a configuration side
effect. No credentials are needed to compile/test. See [Java lifecycle and
platform rules](java-api.md). The library does not execute Kotlin plugins.
