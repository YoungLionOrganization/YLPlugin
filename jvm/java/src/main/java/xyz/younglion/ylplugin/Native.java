// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

package xyz.younglion.ylplugin;
import java.io.InputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Locale;

final class Native {
    static {
        String explicit = System.getProperty("ylplugin.native.path", "");
        if (!explicit.isEmpty()) System.load(Path.of(explicit).toAbsolutePath().toString());
        else {
            String os = System.getProperty("os.name").toLowerCase(Locale.ROOT);
            os = os.contains("win") ? "windows" : os.contains("mac") ? "macos" : os.contains("linux") ? "linux" : os;
            String arch = System.getProperty("os.arch").toLowerCase(Locale.ROOT);
            arch = arch.equals("amd64") ? "x86_64" : arch.equals("aarch64") ? "arm64" : arch;
            String resource = "/natives/" + os + "-" + arch + "/" + System.mapLibraryName("ylplugin_jni");
            try (InputStream input = Native.class.getResourceAsStream(resource)) {
                if (input == null) throw new UnsatisfiedLinkError("No YLPlugin native binary for " + os + "-" + arch + "; include ylplugin-natives or set ylplugin.native.path");
                Path directory = Files.createTempDirectory("ylplugin-jni-");
                Path library = directory.resolve(System.mapLibraryName("ylplugin_jni"));
                Files.copy(input, library);
                directory.toFile().deleteOnExit(); library.toFile().deleteOnExit();
                System.load(library.toAbsolutePath().toString());
            } catch (IOException error) { throw new ExceptionInInitializerError(error); }
        }
        String actual = (String)invoke(0, "version", "{}", null, null, null, null);
        if (!YLPlugin.VERSION.equals(actual)) throw new LinkageError("YLPlugin native " + actual + " does not match SDK " + YLPlugin.VERSION);
    }
    static native long open(String path, String limits);
    static native void close(long handle);
    static native Object invoke(long handle, String operation, String request, byte[] binary, KeyProvider keys, KeyProvider trust, Cancellation cancel);
    private Native() {}
}
