# Java and Kotlin

**0.1.3 additions:** [direct metadata-only reads](Metadata-Only-Reads.md)
request only the 128-byte header and YLSpec frame. TOC, payload and signature
regions are not read. Discovery returns unverified metadata; validation and host
preflight remain separate. Python/npm now bundle the native CLI alongside the
core; Java/Kotlin supply a Java CLI entry point. See
[core/CLI installation and PATH behavior](../installation-cli.md).
These additions are available from 0.1.3.

Published JVM artifacts are `xyz.younglion:ylplugin`, `ylplugin-kotlin` and
`ylplugin-natives`, version 0.1.3. They target JVM 11+; CI uses a JDK 17 test
environment. JNI runs the same native core used by other SDKs.

## Gradle installation

For Java:

```kotlin
repositories { mavenCentral() }
dependencies { implementation("xyz.younglion:ylplugin:0.1.3") }
```

For Kotlin, select the wrapper, which also brings in Java:

```kotlin
repositories { mavenCentral() }
dependencies { implementation("xyz.younglion:ylplugin-kotlin:0.1.3") }
```

The natives JAR is a runtime dependency. These are desktop packages, not an
Android AAR or a Java/Kotlin plugin execution engine.

## Java lifetime

```java
import xyz.younglion.ylplugin.YLPlugin;
import xyz.younglion.ylplugin.Plugin;
import java.nio.charset.StandardCharsets;

try (Plugin plugin = YLPlugin.open("data.ylplugin")) {
    plugin.validate();
    System.out.println(new String(plugin.read("catalog.json"), StandardCharsets.UTF_8));
}
```

`Plugin` is AutoCloseable and synchronized. A Cleaner supplies fallback cleanup,
but try-with-resources is the normal ownership mechanism. Callback close/reentry
is rejected. Do not wait inside a callback for another thread that needs the
same handle lock.

## Kotlin lifetime

```kotlin
import java.nio.file.Path
import xyz.younglion.ylplugin.kotlin.openPlugin
import xyz.younglion.ylplugin.kotlin.readText

openPlugin(Path.of("data.ylplugin")) { plugin ->
    plugin.validate()
    println(plugin.readText("catalog.json"))
}
```

Kotlin helpers use the same Java/JNI handle. `isSupported(host)` evaluates the
preflight status; it does not grant permissions or start a runtime.

## Types, errors and callbacks

Structured reports use Map/List values. Signed integer results use Long; larger
unsigned native results use BigInteger. Profile mask values stay within the
signed-64 range. Resource and permission buffers are byte arrays; use
`Byte.toUnsignedInt` to display values from 0 through 255.

Keys/trust return exactly 32 bytes or null. Native failures are
`YLPluginException` with `getCode()`, while Java callback exceptions remain Java
exceptions. Unicode strings use real UTF-8; invalid surrogates/NUL paths fail.
Read/validate/extract expose cancellation callbacks. The current public
`readSlice` and `verify` convenience methods do not expose cancellation parameters.

## Loading source builds

The loader chooses the packaged OS/architecture resource and copies the trusted
JNI runtime into a private temporary directory. Set
`-Dylplugin.native.path=/absolute/path/libylplugin_jni.so` for a source build,
using DLL/dylib names as appropriate. The SDK version must match the Java facade.
The loader never downloads code and this SDK loading is separate from plugin activation.

Read [Java API](../java-api.md), [Kotlin API](../kotlin-api.md) and
[JVM build/install details](../../jvm/README.md).
