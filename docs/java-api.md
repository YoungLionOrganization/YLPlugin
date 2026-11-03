# Java SDK

Coordinates: `xyz.younglion:ylplugin` and runtime dependency
`xyz.younglion:ylplugin-natives`, version 0.1.2; registry availability is tracked by the publishing workflow. Java 11+ is the
library baseline; CI runs smoke tests on JDK 17. This is a desktop JNI SDK,
not an Android AAR or an implemented Java plugin execution runtime.

```java
import xyz.younglion.ylplugin.YLPlugin;
import xyz.younglion.ylplugin.Plugin;
import java.nio.charset.StandardCharsets;

try (Plugin plugin = YLPlugin.open("data.ylplugin")) {
    plugin.validate();
    String json = new String(plugin.read("catalog.json"), StandardCharsets.UTF_8);
    System.out.println(json);
}
```

`YLPlugin.create` accepts a manifest and resource maps; `YLPlugin.sign` accepts
caller-owned PEM bytes. `Plugin` exposes metadata, inspection, targets,
entrypoints, reads/slices, validation, verification, preflight and extraction.
Structured reports are `Map<String,Object>` / `List` values created by JNI from
the native result; Java does not parse the container. Integer results are Long
or BigInteger when an unsigned value exceeds Long.MAX_VALUE. Permission masks
within the profile's signed-64 range are exact Long values. Resources are bytes;
decode as text only when their content type requires it.

Callbacks implement `KeyProvider.keyFor(String)` or `Cancellation.cancelled()`.
Keys are exactly 32 bytes or null; trust and entitlement are separate parameters.
Callback exceptions remain Java exceptions. Native failures are `YLPluginException`
with `getCode()` matching C statuses. Strings use real UTF-8, including supplementary
characters; invalid UTF-16 surrogates and NUL paths are rejected.

Handle methods are synchronized. An active callback cannot close/reenter that
handle. AutoCloseable is the primary lifetime mechanism; a Cleaner provides
fallback cleanup without exposing a pointer. Do not block callbacks waiting for
another thread that needs the same handle lock.

The native JAR supplies only CI-built platforms. Its loader extracts the trusted
SDK JNI binary into a private temporary directory; this is not untrusted plugin
extraction/activation. For source builds set
`-Dylplugin.native.path=/absolute/path/libylplugin_jni.so` (DLL/dylib on other OSes).
Native/package version mismatches fail and the loader never downloads code.
See [compatibility](compatibility.md) and [publishing](publishing.md).

`permissionBytes(target, optional)` returns a copied byte array. Use
Byte.toUnsignedInt for numeric 0..255 display; byte -1 preserves the 255 bit pattern.
