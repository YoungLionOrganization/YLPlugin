// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

plugins { kotlin("jvm"); `java-library`; `maven-publish` }
java { withSourcesJar() }
kotlin { compilerOptions { jvmTarget.set(org.jetbrains.kotlin.gradle.dsl.JvmTarget.JVM_11) } }
tasks.withType<JavaCompile>().configureEach { options.release.set(11) }
dependencies { api(project(":java")) }
val docs by tasks.registering(Jar::class) { archiveClassifier.set("javadoc"); from(project.file("docs")) }
tasks.register<JavaExec>("smokeTest") {
    dependsOn(tasks.testClasses); classpath = sourceSets.test.get().runtimeClasspath
    mainClass.set("xyz.younglion.ylplugin.kotlin.SmokeTestKt")
    systemProperty("ylplugin.native.path", System.getProperty("ylplugin.native.path", ""))
    args(rootProject.projectDir.parentFile.absolutePath)
}
publishing { publications { create<MavenPublication>("library") { artifactId = "ylplugin-kotlin"; from(components["java"]); artifact(docs) } } }
