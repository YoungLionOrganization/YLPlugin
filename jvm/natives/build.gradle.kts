plugins { `java-library`; `maven-publish` }
java { withSourcesJar(); withJavadocJar() }
publishing { publications { create<MavenPublication>("library") { artifactId = "ylplugin-natives"; from(components["java"]) } } }
