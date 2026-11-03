package xyz.younglion.ylplugin.kotlin

import xyz.younglion.ylplugin.Plugin
import xyz.younglion.ylplugin.YLPlugin
import java.nio.file.Path
import java.nio.charset.StandardCharsets

/** Uses the Java/JNI handle with deterministic cleanup, including on exceptions. */
fun <T> openPlugin(path: Path, block: (Plugin) -> T): T = YLPlugin.open(path.toString()).use(block)
fun Plugin.readText(resource: String): String = String(read(resource), StandardCharsets.UTF_8)
fun Plugin.isSupported(host: Map<String, Any>): Boolean = check(host)["status"] == "SUPPORTED"
