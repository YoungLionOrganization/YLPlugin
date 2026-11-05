// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

package xyz.younglion.ylplugin;

import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/** Native CLI entry point; Maven/Gradle dependencies do not modify the system PATH. */
public final class Cli {
    public static int run(String... args) throws IOException, InterruptedException {
        String explicit = System.getProperty("ylplugin.cli.path", "");
        Path directory = null;
        Path executable;
        if (!explicit.isEmpty()) executable = Path.of(explicit).toAbsolutePath();
        else {
            String os = System.getProperty("os.name").toLowerCase(Locale.ROOT);
            os = os.contains("win") ? "windows" : os.contains("mac") ? "macos" : os.contains("linux") ? "linux" : os;
            String arch = System.getProperty("os.arch").toLowerCase(Locale.ROOT);
            arch = arch.equals("amd64") ? "x86_64" : arch.equals("aarch64") ? "arm64" : arch;
            String filename = os.equals("windows") ? "ylplugin.exe" : "ylplugin";
            try (InputStream input = Cli.class.getResourceAsStream("/natives/" + os + "-" + arch + "/" + filename)) {
                if (input == null) throw new IOException("No bundled YLPlugin CLI for " + os + "-" + arch + "; include ylplugin-natives or set ylplugin.cli.path");
                directory = Files.createTempDirectory("ylplugin-cli-");
                executable = directory.resolve(filename);
                try {
                    Files.copy(input, executable);
                    if (!executable.toFile().setExecutable(true, true)) throw new IOException("Cannot make native CLI executable");
                } catch (IOException error) {
                    Files.deleteIfExists(executable); Files.deleteIfExists(directory); throw error;
                }
            }
        }
        try {
            Process probe = new ProcessBuilder(executable.toString(), "--version").redirectErrorStream(true).start();
            String actual = new String(probe.getInputStream().readAllBytes(), StandardCharsets.UTF_8).trim();
            if (probe.waitFor() != 0 || !YLPlugin.VERSION.equals(actual)) throw new IOException("Native CLI version does not match SDK " + YLPlugin.VERSION);
            List<String> command = new ArrayList<>(); command.add(executable.toString()); command.addAll(List.of(args));
            return new ProcessBuilder(command).inheritIO().start().waitFor();
        } finally {
            if (directory != null) { Files.deleteIfExists(executable); Files.deleteIfExists(directory); }
        }
    }
    public static void main(String[] args) throws Exception { System.exit(run(args)); }
    private Cli() {}
}
