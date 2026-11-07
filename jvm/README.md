# YLPlugin JVM SDK

For **0.1.3**, see [metadata-only discovery](../docs/metadata-only.md)
and [bundled core/CLI installation](../docs/installation-cli.md). These additions
are absent from 0.1.2.

Java 11+ API, Kotlin/JVM 11 helpers and a native runtime JAR. Container, permission,
integrity and compatibility behavior always comes from the same core.

| Artifact | Purpose |
|---|---|
| `xyz.younglion:ylplugin` | Java Plugin / YLPlugin API |
| `xyz.younglion:ylplugin-kotlin` | openPlugin, readText, isSupported |
| `xyz.younglion:ylplugin-natives` | JNI binaries and dependency notices |

Version 0.1.2 applies LGPL-3.0-only to all three publications. JARs include the
license texts, source archives and dependency notices. All three are published on
[Maven Central](https://central.sonatype.com/artifact/xyz.younglion/ylplugin/0.1.3).

## Install

Use Maven Central in your Gradle repositories:

```kotlin
repositories { mavenCentral() }
dependencies {
    implementation("xyz.younglion:ylplugin:0.1.3")
}
```

Kotlin applications can select the wrapper, which also brings in the Java API:

```kotlin
dependencies {
    implementation("xyz.younglion:ylplugin-kotlin:0.1.3")
}
```

The native runtime JAR is a transitive runtime dependency. See
[compatibility](../docs/compatibility.md) for the packaged platforms.

## Build and verify from source

```sh
gradle -p jvm :java:smokeTest :kotlin:smokeTest -Dylplugin.native.path=/absolute/path/libylplugin_jni.so --no-daemon
gradle -p jvm publishAllPublicationsToStagingRepository --no-daemon
```

Build JNI with `YLPLUGIN_BUILD_JNI=ON` and a JDK. Packaging collects actual platform
binaries into the natives module. Explicit paths work during source builds.
Handles are AutoCloseable; Kotlin uses the same handle with `use`.
Read [Java](../docs/java-api.md), [Kotlin](../docs/kotlin-api.md),
[compatibility](../docs/compatibility.md) and [publishing](../docs/publishing.md).
An Android AAR/device validation is not included.
