# Frequently asked questions

## Is a .ylplugin file a ZIP archive?

No. It has a fixed header, embedded YLSpec frame, indexed resource records,
aligned payload and optional signature block. Use the SDK/CLI rather than an
archive utility. Compression applies to individual records.

## Does the SDK execute plugins when I open or inspect a file?

No. SDK loading, metadata inspection, integrity validation, signature trust and
runtime activation are separate operations. Native loading and RCC mounting
require explicit host-controlled runtime integration.

## Which version should I install?

Use 0.1.3 for metadata discovery, bundled CLI installation, validation,
cancellation, Unicode-path and incremental-build fixes. Build source facades and
native libraries from the same version. Format/profile 1.0 is a different
version namespace from the SDK, native ABI and your application's API.

## Are the JS/TS APIs supported in browsers?

The current YLPlugin package is a Node-API desktop SDK. YLSpec's browser support
does not imply browser support for YLPlugin. The WASM example demonstrates a
host-supplied WebAssembly plugin runtime in Node, not a browser SDK build.

## Are Java/Kotlin Android packages available?

The published JVM artifacts contain desktop JNI libraries. No Android AAR,
Android emulator validation, iOS SDK or Windows ARM64 package is claimed.

## Does a valid SHA256 or signature grant permissions?

SHA256 checks resource integrity. Signature verification authenticates bytes
against keys supplied by the host. The host still decides publisher authority,
permission grants, runtime support, budgets and activation policy.

## Is the metadata publisher field trusted?

It is public informational metadata. Associate it with an authenticated key and
your own allowlist only through explicit host policy.

## Why does reading one byte scan a whole resource?

Plaintext slices authenticate the entire resource before returning the selected
bytes. There is no per-chunk authenticated index. 0.1.3 bounds scratch space but
does not omit checks or cache successful hashes across mutable readers.

## Can I decrypt metadata or hide plugin identity with a content key?

Metadata remains public. Content encryption protects selected payload records.
Keys are supplied by the host; encryption does not implement a license server,
payment system, account login or perfect DRM.

## Why is native activation unavailable on Windows?

The SDK does not have the required secure Windows extraction backend. Windows
host SDK operations and packaging tests are supported; that does not imply
untrusted native plugin extraction/activation support.

## Do I need Qt, LeoMiniGames or YLPkg?

Qt is optional and used only by the RCC adapter. The core and language SDKs do
not depend on LeoMiniGames. YLPkg is a separate future installer boundary, not
an implemented requirement for using YLPlugin.

## Can an application link to the LGPLv3 library?

The project uses LGPL-3.0-only. Applications can use its library-linking provisions
and must satisfy the applicable redistribution, source and replacement/relinking
requirements. Read [LICENSING.md](../LICENSING.md), [LICENSE](../LICENSE),
[COPYING](../COPYING) and [SOURCES.md](../SOURCES.md); third-party terms remain separate.
