# Building, testing and installing

Requirements: CMake 3.26+, C11/C++20 compilers, YLSpec 0.1.2, OpenSSL 3+, Zstd,
nlohmann-json 3.11+, and threads. Python 3.11+ is needed for integration tests.
QtCore 6.4+ is optional; the core, CLI, C/C++ SDK and Python SDK have no Qt
dependency. No dependency is downloaded during configure.

Ubuntu/Debian dependencies: `cmake ninja-build g++ libssl-dev libzstd-dev
nlohmann-json3-dev`; optional `qt6-base-dev`. macOS: Homebrew CMake/Ninja,
openssl@3, zstd, nlohmann-json, optional qt. Windows: MSVC C++20 and vcpkg
openssl/zstd/nlohmann-json, with its CMake toolchain. Windows native extraction
is currently unsupported; do not register the native adapter as usable there.

```sh
git clone https://github.com/YoungLionOrganization/YLSpec.git ../YLSpec
git -C ../YLSpec checkout 74ef1470105566b812c8a712ce56848f98c277cb
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DYLPLUGIN_YLSPEC_SOURCE=../YLSpec -DYLPLUGIN_QT=OFF
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
build/ylplugin inspect examples/artifacts/demo.ylplugin
```

Alternatively install YLSpec first and omit `YLPLUGIN_YLSPEC_SOURCE`; CMake uses
`find_package(YLSpec 0.1.2 CONFIG)`. Its normal dependency license/notices are
installed alongside the SDK when built from source. To build/test RCC memory
support set `-DYLPLUGIN_QT=ON`. Checked-in RCC/container fixtures require no RCC
compiler to run tests. To build another example use `RCC=/path/to/rcc sh
examples/build.sh`; its new output directory defaults to `build/rcc-example`.
Pass a different new directory as the first argument for another run. Checked-in
fixtures are preserved.

```sh
cmake --install build --prefix /absolute/sdk-prefix
cmake -S examples -B build/consumer -DCMAKE_PREFIX_PATH=/absolute/sdk-prefix
cmake --build build/consumer --parallel 4
build/consumer/read_cpp examples/artifacts/demo.ylplugin
```

Consumers use `find_package(YLPlugin 0.1 CONFIG REQUIRED)` and link
`YLPlugin::ylplugin`; optional Qt integration is `YLPlugin::ylplugin_qt` in the
installed export (`YLPlugin::qt` is the in-tree alias). Shared library runtime
search paths/DLL deployment are the host's responsibility. The project does not
depend on LeoMiniGames or require its source in SDK consumers.

In the prepared cloud workspace:

```sh
source /workspace/younglion-env/native.sh
cmake -S /workspace/YLPlugin -B /workspace/YLPlugin/build -G Ninja \
  -DYLPLUGIN_YLSPEC_SOURCE=/workspace/YLSpec -DYLPLUGIN_QT=ON
cmake --build /workspace/YLPlugin/build --parallel 4
ctest --test-dir /workspace/YLPlugin/build --output-on-failure
```

For ASan/UBSan add `-DYLPLUGIN_SANITIZE=ON -DYLSPEC_SANITIZE=ON` in a separate
Debug build. On GCC/Linux, CMake preloads libasan AND libstdc++ for ctypes tests
so the C++ exception interceptor exists when Python dlopens the library. Leak
detection stays enabled for native tests, but is disabled for the uninstrumented
Python interpreter process; address/undefined-behavior checks remain enabled.
Other sanitizer/compiler hosts may require equivalent launcher configuration.

For libFuzzer use Clang and `-DYLPLUGIN_FUZZ=ON -DYLPLUGIN_SANITIZE=ON
-DYLSPEC_SANITIZE=ON -DBUILD_TESTING=OFF`, then build `ylplugin_fuzz` and run it
with a corpus copied from `tests/vectors/*.ylplugin`. CI specifies a bounded
60-second run. The core tests also execute a deterministic 3000-mutation binary
fuzz campaign on every regular/sanitized run. This is not exhaustive fuzz coverage.

CI declares Linux x64/ARM64, macOS and Windows builds, sanitizer tests and
libFuzzer. A checked-in workflow is not evidence of a successful remote run.
Android/iOS and Windows ARM64 need actual SDK/toolchain/device validation before
release; they are not claimed tested by the local Linux run. Core format types
are fixed-width, serialized explicitly and independent of native struct layout.

Python packaging: `python -m pip install .` installs the pure ctypes facade;
provide the separately built shared library using Library(path) or YLPLUGIN_LIBRARY.
The Python wheel does not bundle SDK binaries or claim platform wheel coverage.
No package registry or GitHub release publishing workflow is configured.
