# SDK and platform compatibility

File format 1.0 and profile 1.0 are portable; execution availability belongs to
the host. SDK 0.1.2 carries LGPLv3 licensing and remains experimental. It does not alter the released
v0.1.0 bytes or claim a stable C++ ABI.

| Layer | Baseline / limitation |
|---|---|
| C/C++ core | C11 dependency / C++20 SDK, 64-bit serialized offsets |
| Node | Node 20+, Node-API 8, ESM and CommonJS; not a browser SDK |
| Java/Kotlin | Java 11 / JVM target 11; tests on JDK 17; desktop JNI |
| Python | CPython 3.11+, ctypes facade and platform-specific native wheels |
| Linux prebuilds | GNU x86_64/ARM64 built on Ubuntu 24.04; wheel policy checked by auditwheel |
| macOS prebuilds | Actual x86_64/ARM64 CI targets, deployment target 11.0 |
| Windows prebuilds | x86_64, static SDK runtime; secure plugin extraction still unsupported |
| Android/iOS/Windows ARM64 | Format/source targets; no new packaged/native device support claimed |

The packaging manifest lists actual collected targets. A matrix definition is
not proof that a run passed. Linux wheel manylinux_2_39 tags do not imply a
manylinux_2_28 baseline; npm/JNI GNU binaries also need the build-compatible C++
runtime. Musl, browsers and unlisted architectures require source work rather
than silently loading an incompatible prebuild.

SDK binary loading is version-checked and separate from plugin activation.
Loading the trusted JNI/Node/ctypes runtime does not execute a plugin entrypoint.
Preflight takes the host's real API, OS, architecture, ABI, runtime versions,
capabilities, explicit grants and budgets. A matching host ID alone is insufficient.

Node integer results beyond 2^53-1 are bigint. Java reports signed integers as
Long and larger unsigned values as BigInteger; Python uses exact int. Permission
integer masks remain restricted by the profile to signed-64 nonnegative values.
Byte masks and resource buffers remain numeric bytes, not decimal strings.
