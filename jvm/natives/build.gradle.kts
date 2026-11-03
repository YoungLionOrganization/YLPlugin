// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

plugins { `java-library`; `maven-publish` }
java { withSourcesJar(); withJavadocJar() }
publishing { publications { create<MavenPublication>("library") { artifactId = "ylplugin-natives"; from(components["java"]) } } }
