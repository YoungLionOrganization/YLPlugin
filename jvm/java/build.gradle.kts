plugins { `java-library`; `maven-publish` }
java { withSourcesJar(); withJavadocJar() }
tasks.withType<JavaCompile>().configureEach { options.release.set(11); options.encoding = "UTF-8" }
tasks.withType<Javadoc>().configureEach { (options as StandardJavadocDocletOptions).addStringOption("Xdoclint:none", "-quiet") }
dependencies { runtimeOnly(project(":natives")) }
tasks.register<JavaExec>("smokeTest") {
    dependsOn(tasks.testClasses); classpath = sourceSets.test.get().runtimeClasspath
    mainClass.set("xyz.younglion.ylplugin.SmokeTest")
    systemProperty("ylplugin.native.path", System.getProperty("ylplugin.native.path", ""))
    args(rootProject.projectDir.parentFile.absolutePath)
}
publishing { publications { create<MavenPublication>("library") { artifactId = "ylplugin"; from(components["java"]) } } }
