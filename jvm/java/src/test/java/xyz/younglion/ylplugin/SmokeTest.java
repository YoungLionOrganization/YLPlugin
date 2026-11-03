// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

package xyz.younglion.ylplugin;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Map;
import java.util.List;
import java.util.Arrays;
import java.security.KeyPairGenerator;
import java.util.Base64;
import java.nio.charset.StandardCharsets;

public final class SmokeTest {
    static int checks;
    static void check(boolean value) { checks++; if (!value) throw new AssertionError("Java check " + checks); }
    static void rejected(Runnable operation, int code) {
        try { operation.run(); throw new AssertionError("native operation accepted"); }
        catch (YLPluginException expected) { check(expected.getCode() == code); }
    }
    @SuppressWarnings("unchecked") public static void main(String[] args) throws Exception {
        Path root = Path.of(args[0]), work = Files.createTempDirectory("ylplugin-java-").toRealPath();
        String fixture = root.resolve("examples/artifacts/data.ylplugin").toString();
        String manifest = Files.readString(root.resolve("examples/data/plugin.ylspec"));
        Map<String,Object> resource = Map.of("path", "catalog.json", "file", root.resolve("examples/data/catalog.json").toString(), "media", "application/json");
        Map<String,Object> host = Map.of("id", "xyz.example.data-viewer", "api", "1.0", "os", "linux", "arch", "x86_64", "runtimes", Map.of("data", "1.0"), "memory", 67108864);
        try {
            check(YLPlugin.version().equals(YLPlugin.VERSION));
            try (Plugin p = YLPlugin.open(fixture)) {
                p.validate(); check(new String(p.read("catalog.json"), StandardCharsets.UTF_8).contains("42"));
                Map<String,Object> permission = (Map<String,Object>)p.targets().get(1).get("required_permissions");
                check(permission.get("value").equals(Long.MAX_VALUE));
                check(Arrays.equals(p.permissionBytes(0, true), new byte[]{37,8,(byte)255,0}));
                check(p.check(host).get("status").equals("SUPPORTED"));
                check(p.readSlice("catalog.json", 0, 1)[0] == '{');
                rejected(() -> p.read("catalog.json", null, () -> true), 10);
                rejected(() -> p.readSlice("catalog.json", -1, 1), 3);
            }
            YLPlugin.create(work.resolve("roundtrip").toString(), manifest, List.of(resource));
            check(Arrays.equals(Files.readAllBytes(work.resolve("roundtrip")), Files.readAllBytes(Path.of(fixture))));
            String unicode = manifest.replace("version = \"1.0.0\"", "version = \"1.0.0\"\npublisher = \"Cavanşir 🦁\"");
            YLPlugin.create(work.resolve("unicode").toString(), unicode, List.of(resource));
            try (Plugin p = YLPlugin.open(work.resolve("unicode").toString())) { check(p.metadata().contains("Cavanşir 🦁")); }
            byte[] key = new byte[32]; Arrays.fill(key, (byte)37);
            KeyProvider keys = id -> id.equals("content") ? key : null;
            Map<String,Object> encrypted = Map.of("path", "catalog.json", "file", resource.get("file"), "media", "application/json", "compression", 1, "key_id", "content");
            YLPlugin.create(work.resolve("encrypted").toString(), manifest, List.of(encrypted), keys, null);
            try (Plugin p = YLPlugin.open(work.resolve("encrypted").toString())) {
                rejected(() -> p.read("catalog.json"), 8);
                rejected(() -> p.read("catalog.json", id -> new byte[32], null), 6);
                check(p.read("catalog.json", id -> { rejected(p::close, 3); return key; }, null).length > 0);
                try { p.read("catalog.json", id -> { throw new IllegalStateException("provider failure"); }, null); throw new AssertionError(); }
                catch (IllegalStateException expected) { check(expected.getMessage().equals("provider failure")); }
                p.validate(keys, null);
            }
            java.security.KeyPair pair = KeyPairGenerator.getInstance("Ed25519").generateKeyPair();
            String pem = "-----BEGIN PRIVATE KEY-----\n" + Base64.getMimeEncoder(64, new byte[]{'\n'}).encodeToString(pair.getPrivate().getEncoded()) + "\n-----END PRIVATE KEY-----\n";
            YLPlugin.sign(work.resolve("encrypted").toString(), work.resolve("signed").toString(), "publisher", pem.getBytes(StandardCharsets.US_ASCII));
            byte[] der = pair.getPublic().getEncoded(), pub = Arrays.copyOfRange(der, der.length - 32, der.length);
            try (Plugin p = YLPlugin.open(work.resolve("signed").toString())) { p.verify(id -> pub); rejected(() -> p.verify(id -> null), 7); }
            Plugin closed = YLPlugin.open(fixture); closed.close(); closed.close();
            try { closed.inspect(); throw new AssertionError(); } catch (IllegalStateException expected) { check(true); }
            try (Plugin unknown = YLPlugin.open(root.resolve("tests/vectors/unsupported-profile.ylplugin").toString())) { rejected(unknown::targets, 2); }
            rejected(() -> YLPlugin.open(fixture + "\0bad"), 3);
            System.out.println(checks + " Java/JNI checks passed");
        } finally {
            try (java.util.stream.Stream<Path> files = Files.walk(work)) {
                files.sorted(java.util.Comparator.reverseOrder()).forEach(path -> { try { Files.delete(path); } catch (java.io.IOException error) { throw new java.io.UncheckedIOException(error); } });
            }
        }
    }
}
