# Licensing and Sources

Original YLPlugin code, SDK bindings, tools, tests, examples and documentation
use **LGPL-3.0-only**. Copyright is held by Cavanshirpro. The full LGPLv3 text is
in LICENSE, with its incorporated GPLv3 terms in COPYING. The software is
provided under the warranty/liability terms of those texts.

## Distribution responsibilities

Applications may use the library under LGPLv3's linking provisions. Distribution
must retain required notices/license texts, provide required corresponding
source and preserve the applicable ability to modify/relink/replace covered
library components. Do not prohibit reverse engineering for debugging those
modifications. The complete license controls these obligations.

Native Node/JNI/ctypes modules can statically embed the core. Rebuilding and
replacing the complete covered module is supported. The loader's SDK version
check requires a compatible version string; it does not require an official
binary hash or project signing key.

## Sources supplied with packages

Native distributions contain license texts, project source archives and the
actual YLSpec checkout's source archive. npm platform license directories,
Python distribution license payloads and JAR notices retain these materials.
The release source archive and manifests identify the implementation commit.

The source includes headers, core, bindings, build scripts and examples.
SOURCES.md identifies pinned YLSpec revisions and upstream dependency locations;
the build guide describes static/shared and language configurations. Rebuilding
does not require the maintainer's publishing credentials or private signing keys.

## Third-party terms

YLSpec, tomlc17, OpenSSL, Zstandard, nlohmann-json, Node/JDK headers and optional
Qt retain their own licenses/copyrights. This project does not relicense those
components. Exact platform license texts are collected from actual dependency
builds. Qt is not linked into language SDK bundles and remains optional.

Version 0.1.2 applied the owner-selected license to distribution metadata and
source notices. Historical registry files may show earlier metadata; current
LGPL-bearing distributions start with 0.1.2. Use matching release/source/license
files when redistributing rather than assuming publication itself supplied a grant.

Read [LICENSING.md](../../LICENSING.md), [LICENSE](../../LICENSE),
[COPYING](../../COPYING), [SOURCES.md](../../SOURCES.md) and
[third-party notices](../../THIRD_PARTY_NOTICES.md).
