// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

package xyz.younglion.ylplugin;
import java.util.Map;
import java.util.List;

/** Static native SDK operations; no plugin code is executed by these methods. */
public final class YLPlugin {
    public static final String VERSION = "0.1.3";
    public static String version() { return (String)Native.invoke(0, "version", "{}", null, null, null, null); }
    public static Plugin open(String path) { return new Plugin(path, Map.of()); }
    public static Plugin open(String path, Map<String, ?> limits) { return new Plugin(path, limits); }
    public static void create(String output, String manifest, List<? extends Map<String, ?>> resources) { create(output, manifest, resources, null, null); }
    public static void create(String output, String manifest, List<? extends Map<String, ?>> resources, KeyProvider keys, Cancellation cancel) {
        Native.invoke(0, "create", Json.encode(Map.of("output", output, "manifest", manifest, "resources", resources)), null, keys, null, cancel);
    }
    public static void sign(String input, String output, String keyId, byte[] privatePem) {
        Native.invoke(0, "sign", Json.encode(Map.of("input", input, "output", output, "keyId", keyId)), privatePem, null, null, null);
    }
    private YLPlugin() {}
}
