# Installation and Builds

## Choose a distribution

| Consumer | Published 0.1.2 distribution |
|---|---|
| Python 3.11+ | `ylplugin`, platform-specific ctypes runtime wheels |
| Node 20+ JS/TS | `@younglionorganization/ylplugin`, native Node-API prebuilds |
| Java 11+ | `xyz.younglion:ylplugin`, JNI runtime dependency |
| Kotlin/JVM 11+ | `xyz.younglion:ylplugin-kotlin`, Java/JNI dependencies |
| C/C++ or unsupported platform | Source build and CMake package export |

Python source installs and the universal GitHub facade wheel do not bundle a
native SDK. Published platform wheels do. JVM native-library extraction into a
private temporary directory loads the trusted SDK binary, not a plugin payload.

Supported packaged architectures are Linux/macOS x86_64 and ARM64 plus Windows
x86_64. Linux wheels use manylinux_2_39; npm/JNI GNU binaries also need compatible
glibc/C++ runtimes. Musl, Windows ARM64, Android and iOS require additional source
work/validation. Desktop JVM support does not imply Android support.

## Build the core

From a YLPlugin checkout, install CMake 3.26+, a C11/C++20 compiler, Ninja,
OpenSSL 3+, Zstandard, nlohmann-json 3.11+ and Python for tests. Obtain the
compatible YLSpec source:

```sh
git clone https://github.com/YoungLionOrganization/YLSpec.git ../YLSpec
git -C ../YLSpec checkout 74ef1470105566b812c8a712ce56848f98c277cb
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DYLPLUGIN_YLSPEC_SOURCE=../YLSpec -DYLPLUGIN_QT=OFF
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

Ubuntu dependencies include `ninja-build g++ libssl-dev libzstd-dev
nlohmann-json3-dev`. Check your distribution's CMake version. macOS uses Homebrew
dependencies; Windows uses MSVC and a vcpkg toolchain. The core configure step
does not download dependencies. Qt 6.4+ is optional for the RCC adapter.

```sh
cmake --install build --prefix /absolute/sdk-prefix
```

Consumers link:

```cmake
find_package(YLPlugin CONFIG REQUIRED)
target_link_libraries(my_host PRIVATE YLPlugin::ylplugin)
```

## Language source builds

Node builds need official matching headers and, on Windows, `node.lib`. JNI
needs a JDK. Enable `YLPLUGIN_BUILD_NODE` and/or `YLPLUGIN_BUILD_JNI`; supply
`NODE_INCLUDE_DIR` and `JAVA_HOME`. Standalone ctypes bundles additionally use
`YLPLUGIN_LIBRARY_TYPE=STATIC` and `YLPLUGIN_BUILD_PYTHON_RUNTIME=ON`.

The language CI helper combines static dependencies into modules, runs API
tests, collects license/source notices and packages actual platform binaries.
It uses the separately pinned YLSpec 0.1.4 commit recorded in SOURCES.md. Follow
the source [build guide](../building.md) and workflow rather than assuming the
basic shared-core command also produces every registry package.

## Version and loader selection

The default branch prepares 0.1.3. Native/runtime and facade versions must match
even though C ABI version 1 and file/profile 1.0 remain unchanged. Explicit
loader selectors are `YLPLUGIN_LIBRARY`/`Library(path)`, `YLPLUGIN_NODE_PATH`
and JVM `ylplugin.native.path`. No SDK import downloads executable code.

From 0.1.3, VERSION changes invalidate CMake configuration automatically. Older
build directories should be explicitly reconfigured before rebuilding.
