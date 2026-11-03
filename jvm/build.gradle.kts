import org.gradle.api.publish.maven.MavenPublication
plugins { base; kotlin("jvm") version "2.4.20" apply false }
version = file("../VERSION").readText().trim()
allprojects { group = "xyz.younglion"; version = rootProject.version; repositories { mavenCentral() } }
subprojects {
    plugins.withId("java") {
        extensions.configure<JavaPluginExtension> {
            sourceCompatibility = JavaVersion.VERSION_11
            targetCompatibility = JavaVersion.VERSION_11
        }
        tasks.withType<JavaCompile>().configureEach { options.release.set(11); options.encoding = "UTF-8" }
    }
    apply(plugin = "maven-publish")
    extensions.configure<PublishingExtension> {
        repositories { maven { name = "staging"; url = rootProject.layout.buildDirectory.dir("maven-repository").get().asFile.toURI() } }
        publications.withType<MavenPublication>().configureEach {
            pom {
                name.set("YLPlugin " + project.name)
                description.set("Native YLPlugin container SDK for Java and Kotlin; not a runtime sandbox")
                url.set("https://github.com/YoungLionOrganization/YLPlugin")
                licenses { license { name.set("No license grant; project license decision pending"); url.set("https://github.com/YoungLionOrganization/YLPlugin/blob/main/LICENSING.md") } }
                developers { developer { id.set("YoungLionOrganization"); name.set("YoungLionOrganization"); email.set("cavanshirpro@gmail.com") } }
                scm { connection.set("scm:git:https://github.com/YoungLionOrganization/YLPlugin.git"); url.set("https://github.com/YoungLionOrganization/YLPlugin") }
            }
        }
    }
    tasks.withType<Zip>().configureEach {
        isPreserveFileTimestamps = false; isReproducibleFileOrder = true
        from(rootProject.file("../LICENSING.md")) { into("META-INF") }
        from(rootProject.file("../THIRD_PARTY_NOTICES.md")) { into("META-INF") }
        from(rootProject.file("natives/src/main/resources/licenses")) { into("META-INF/licenses") }
    }
    tasks.withType<Jar>().configureEach { manifest.attributes("Implementation-Version" to project.version) }
}
