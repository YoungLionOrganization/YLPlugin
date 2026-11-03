# YLPlugin licensing

Copyright (C) 2026 Cavanshirpro.

Original YLPlugin code, language bindings, tools, tests, examples and documentation
are licensed under the **GNU Lesser General Public License, version 3 only**
(`LGPL-3.0-only`). This license is selected by the project owner.

The full LGPLv3 text is in [LICENSE](LICENSE). Its incorporated GPLv3 terms are
in [COPYING](COPYING). The software is provided without warranty, as described
in those texts. SPDX identifiers in source files identify the same license.

## Using and distributing the library

Applications may use YLPlugin under LGPLv3's library-linking provisions. When
redistributing it or a combined work, retain the notices and license texts,
provide the required corresponding source, and preserve users' ability to modify
the library and relink or replace the covered library components as required by
LGPLv3. Do not prohibit reverse engineering for debugging those modifications.
The full license controls; this paragraph is a practical introduction.

Native Node/JNI/ctypes modules can embed the YLPlugin core statically. Rebuilding
and replacing the complete covered module is supported; the host SDK version
check requires the same version string and does not require an official binary
hash or publisher signature. See [SOURCES.md](SOURCES.md) for source and build
instructions. Packaged native distributions include source archives and license
texts alongside their dependency notices.

## Third-party components

YLSpec, tomlc17, OpenSSL, Zstandard, nlohmann-json, Node/JDK headers and optional
Qt retain their own copyright and licensing terms. This change does not relicense
them. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and the collected
dependency license files in binary packages.

Version 0.1.2 applies the owner-selected license to source and package metadata.
Older registry files contain historical license metadata and are immutable;
use 0.1.2 or later for distributions carrying this license and its notices.
