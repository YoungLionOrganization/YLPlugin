# Kotlin SDK

Coordinates: `xyz.younglion:ylplugin-kotlin`, version 0.1.2; registry availability is tracked by the publishing workflow. The
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
