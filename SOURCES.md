# Source and rebuilding YLPlugin

Original project code is LGPL-3.0-only. Full license texts are supplied in
`LICENSE` (LGPLv3) and `COPYING` (the incorporated GPLv3 terms).

The canonical source repository is https://github.com/YoungLionOrganization/YLPlugin.
Each release source archive records its exact commit in `release-manifest.json`.
The language distributions' `libraries.json` records the matching implementation
commit. Native packages also carry `YLPlugin-source-<version>.tar.gz` and the
actual YLSpec checkout's source archive with their licenses and notices.

The source includes the core, public headers, Node/JNI/ctypes bindings, build
scripts and examples. Rebuild instructions are in `docs/building.md`;
`tools/configure_libraries.py` and `.github/workflows/libraries.yml` specify the
platform SDK configuration. The native libraries can be replaced with modified
builds using the packaged paths or the documented explicit library path settings.
Keep the SDK version string compatible with the facade being used.

The language pipeline uses YLSpec commit
`8a07a17295eb7065e402a49e96200a551b23f752` from
https://github.com/YoungLionOrganization/YLSpec, including its vendored tomlc17.
The core conformance pipeline also checks compatibility with YLSpec commit
`74ef1470105566b812c8a712ce56848f98c277cb`.

Other dependency sources and terms remain upstream:

| Dependency | Source |
|---|---|
| OpenSSL | https://github.com/openssl/openssl |
| Zstandard | https://github.com/facebook/zstd |
| nlohmann-json | https://github.com/nlohmann/json |
| Node-API headers | https://github.com/nodejs/node; `tools/node_headers.py` records the exact Node release |
| JNI headers | The JDK selected by the build workflow |
| Qt, optional | https://code.qt.io/; not linked into language SDK bundles |

Collected dependency license files identify the actual platform distributions.
No package import downloads executable code. Rebuilding does not require access
to the project's signing keys or publishing credentials.
