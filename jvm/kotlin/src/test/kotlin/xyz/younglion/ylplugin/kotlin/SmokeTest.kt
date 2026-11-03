package xyz.younglion.ylplugin.kotlin
import xyz.younglion.ylplugin.YLPlugin
import java.nio.file.Path

fun main(args: Array<String>) {
    val path = Path.of(args[0], "examples", "artifacts", "data.ylplugin")
    openPlugin(path) { plugin ->
        check(plugin.readText("catalog.json").contains("42"))
        check(plugin.targets()[1]["required_permissions"] is Map<*, *>)
        plugin.validate()
    }
    check(YLPlugin.version() == YLPlugin.VERSION)
    println("Kotlin/JNI resource and use lifecycle checks passed")
}
