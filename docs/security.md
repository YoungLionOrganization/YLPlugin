# Security architecture and limits

Untrusted container bytes are data. Parsing, metadata inspection, profile
validation and preflight never load plugin code. The trust boundaries are:

1. Bounds/structure: strict locators, canonical non-overlapping regions, zero
   reserved bytes/padding, unknown mandatory-feature rejection, bounded metadata/TOC.
2. Identity/schema: existing YLSpec frame decoder plus native plugin profile.
3. Integrity: per-record SHA256 after authenticated decrypt/decompress; bounded
   Zstd output/window, exactly one frame, no dictionary/trailing streams.
4. Authenticity: separate full-container signature verification against a
   caller-owned trust store. Unknown/revoked keys fail. Key rotation uses stable
   IDs with host-controlled validity/revocation; no network or TOFU fallback.
5. Entitlement: synchronous key-provider callback for a 32-byte AES key, keyed by
   ID; missing keys are NEEDS_LICENSE. AEAD authenticates before decompression.
6. Authorization: the host interprets and grants capabilities/permissions,
   publisher privileges, native execution and runtime access.

`validate` verifies content/profile, not publisher authenticity. `verify` checks
stored bytes/signature, without requiring decryption keys. `check` reports
signature state; SUPPORTED may coexist with UNVERIFIED/UNSIGNED if the host did
not require signatures. Hosts requiring signed content MUST set require_signature
and supply a trust provider. With a trust provider, any invalid/revoked signature
blocks preflight. Signature validity does not imply safe native code or safe RCC.

The signature algorithm and AEAD byte contracts are normative in
`specification.md`. All cryptography is OpenSSL EVP/RAND. No hand-written cipher,
MAC, Ed25519 arithmetic or payment/account protocol is used. Key material is not
stored in metadata. Core key arrays are cleansed; user callbacks own their secret
storage. Python byte objects cannot promise erasure. Secure key storage, offline
entitlement, device-bound unwrapping and future YoungLion Account/API integration
belong behind the host key callback. Encryption is not perfect DRM: a host that
can decrypt content can expose plaintext. No network/account/payment backend is
part of YLPlugin.

Extraction is descriptor-anchored on POSIX, O_NOFOLLOW for every ancestor and
created directory, O_EXCL for leaf files, no link entries, 0700 staging directories,
0600 files, integrity check before publication, per-record cancellation, fsync of
files, and atomic no-replace directory rename. The destination must be new. Its
parent must already exist, be owned by the effective user and not writable by
group/others. This prevents a different user replacing staging names during
publication. Native caches must be private, scoped by host/plugin/session, and
remain under host control until unload. Same-user hostile processes and a
compromised host are outside this boundary. No resource is made executable by a
wire mode bit. DLL/SO mapping does not require executable file mode.

The container builder/signing outputs also use new private sibling staging and
atomic no-replace hard-link publication. Use a trusted output parent; CLI authors
choose these paths. Filesystems without hard links/no-replace rename fail safely.
This is transactional visibility, not a power-loss durability guarantee. Abrupt
process termination can leave a private `.ylplugin-*` staging directory. Remove
only abandoned directories owned by the host after ensuring no active session
uses them. Never execute partial files; a truncated container fails length checks.
No automatic salvage path, cross-package deduplication, shared writable cache,
or in-place update is implemented.

The reader holds its file handle across reads and serializes seeking. Callers
must keep backing files/custom readers stable and not mutate the underlying file
in place; returned metadata/views are immutable. Resource hashes are checked
against the loaded TOC before bytes are returned. Custom reader callbacks must
support concurrent requests or synchronize internally. The host must serialize
close against reads and obey view lifetime rules. Cancellation is cooperative,
not a mechanism for terminating arbitrary native code.

This is an experimental implementation, with no independent cryptographic audit,
no OS sandbox and no general validator for Qt's RCC internals. See `test-report.md`
for exact executed checks and remaining platform validation. Do not deploy it as
a production security boundary before completing the release audit in RELEASES.md.

## Language boundary checks in 0.1.1

Node/JNI callbacks are synchronous and per-handle reentry/close is rejected. Java
uses AutoCloseable/Cleaner and UTF-8 conversion with surrogate checks; Python
validates integer widths and NUL paths before ctypes and serializes close/read.
These rules prevent integer wrapping and freeing an active native operation;
they do not sandbox native code or terminate blocking host callbacks.

Native wheels/prebuilds/JAR resources contain trusted SDK code and dependency
notices. They never fetch a binary on import. Registry publication verifies CI
provenance, package identities and hashes before using organization secrets;
permission to publish a library is not permission to execute an untrusted plugin.

Static SDK modules export only their frontend/C ABI entrypoints; embedded core
and crypto symbols remain private to avoid interposition with a host OpenSSL build.
This controls linkage, not the trust or isolation of activated plugin code.
