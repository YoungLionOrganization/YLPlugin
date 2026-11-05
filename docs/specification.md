# YLPlugin container 1.0 (experimental draft)

## Direct YLSpec discovery

The existing format layout supports reading only the 128-byte header and the
YLSpec frame located at offset 128. SDK 0.1.3 adds dedicated operations that use
this layout without parsing the table of contents or reading payload/signature
regions. This is an additive SDK feature; the container format and profile stay
at 1.0. See [metadata-only reads](metadata-only.md) for exact read scope, limits,
all language entry points and trust boundaries.

Normative language: English. MUST, MUST NOT and SHOULD are requirements.
This draft is implemented by library 0.1.0; it is not a stable or audited release.
The container, YLSpec frame, plugin profile, SDK ABI, runtime ABI and host API
have independent versions. No YLPkg installation is required.

## Byte contract

All unsigned integers are little endian. Structures MUST be serialized by field,
never by copying a compiler struct. Header size is 128 bytes:

| Offset | Size | Value |
|---:|---:|---|
| 0 | 8 | `59 4c 50 4c 55 47 00 00` (`YLPLUG\0\0`) |
| 8 | 2 | container major = 1 |
| 10 | 2 | container minor = 0 |
| 12 | 4 | header bytes = 128 |
| 16 | 8 | mandatory features: Zstd=1, AES-GCM=2, signature=4 |
| 24 | 8 | exact file length (trailing bytes forbidden) |
| 32 | 8 | metadata offset = 128 |
| 40 | 8 | metadata frame length |
| 48 | 8 | TOC offset = align8(metadata end) |
| 56 | 8 | TOC length |
| 64 | 8 | record count |
| 72 | 8 | payload offset = align8(TOC end) |
| 80 | 8 | payload length, including alignment gaps |
| 88 | 8 | signature offset, or zero |
| 96 | 8 | signature length = 128, or zero |
| 104 | 24 | reserved zero |

Unknown mandatory bits, versions or nonzero reserved bytes MUST fail closed.
Every region MUST fit the declared and physical file length using subtraction
checks before addition. Payload ends at EOF or the signature. Metadata is an
unmodified YLSpec 0.1 frame from `yl_pack`; its 32-byte header precedes a variable
length UTF-8 TOML 1.1 document. `yl_read_at` receives the exact frame region;
its consumed size MUST equal the declared metadata length. Metadata is public.

## Indexed records

TOC consists of `record_count` records sorted by bytewise ASCII resource path.
Each record is `align8(128 + path_len + media_len + variant_len + key_id_len)`
bytes. Strings follow the fixed part in that order, without NUL terminators.
String lengths are u16; path is 1..1024 bytes; other strings are 0..255 bytes
(media is nonempty). Padding and reserved bytes MUST be zero.

| Offset | Size | Field |
|---:|---:|---|
| 0 | 4 | total record bytes |
| 4 | 1 | compression: store=0, Zstd=1 |
| 5 | 1 | encryption: none=0, AES-256-GCM=1 |
| 6 | 1 | extraction: auto=0, direct-required=1, extract-required=2 |
| 7 | 1 | reserved |
| 8 | 8 | absolute payload offset |
| 16 | 8 | stored size |
| 24 | 8 | plaintext uncompressed size |
| 32 | 4 | alignment: power of two, 8..4096 |
| 36 | 2 | path length |
| 38 | 2 | media type length |
| 40 | 2 | platform/runtime variant association length |
| 42 | 2 | encryption key ID length |
| 44 | 4 | reserved |
| 48 | 32 | SHA-256 of decoded plaintext |
| 80 | 12 | GCM nonce, or zeros |
| 92 | 16 | GCM tag, or zeros |
| 108 | 20 | reserved |

Payload records MUST appear in TOC order at the next aligned offset. Gaps MUST
be zero. Duplicate paths, case-insensitive aliases and file/directory prefix
collisions MUST be rejected. Paths use portable ASCII `[A-Za-z0-9._/-]`, no
absolute paths, empty/dot/dot-dot components, trailing dot, Windows device names,
colon, backslash or components exceeding 255 bytes. Symlinks/hardlinks and
executable mode bits have no wire representation. Empty resources are allowed.

Readers MUST bound TOC/metadata/count, each decoded record and the aggregate
decoded size before allocating or decompressing. Defaults: metadata 16 MiB,
TOC 8 MiB, 16384 records, individual decoded/stored record 64 MiB, aggregate
1 GiB, expansion ratio 256 with a 1 MiB allowance. Limits are host-configurable.
A package may exceed 4 GiB using multiple records and increased aggregate limits;
offsets and I/O are 64-bit. Streaming is per record; arbitrary-size single-record
decoding and chunked resources are not implemented in 1.0.

Store MUST have stored_size == plaintext_size. Zstd MUST contain exactly one
complete standard frame, no trailing/concatenated frames or dictionaries; its
known content size MUST match plaintext_size. Decoding MUST respect output and
window limits. RCC defaults to store to avoid double compression.

## Authentication and confidentiality

AES-256-GCM encrypts the compressed record. Nonces MUST be generated with the
cryptographic RNG independently for each encryption, never derived from a path.
AAD is the complete TOC record with bytes 92..107 zeroed, including strings and
padding. A 32-byte key is supplied by a synchronous host callback keyed by key
ID. Authentication MUST complete before decompression or releasing plaintext.
SHA-256 MUST match before decoded bytes are returned. Metadata contains no keys.
Repacking encrypted records generates fresh nonces. Encryption is not perfect DRM.

The optional 128-byte signature block is:

| Offset | Size | Field |
|---:|---:|---|
| 0 | 8 | `YLSIG\0\0\0` |
| 8 | 4 | algorithm = 1 |
| 12 | 4 | reserved zero |
| 16 | 32 | ASCII key ID, NUL padded (1..31 bytes) |
| 48 | 8 | signed prefix length = signature_offset + 64 |
| 56 | 8 | reserved zero |
| 64 | 64 | Ed25519 signature |

Algorithm 1 signs `"YLPlugin-SHA512-Ed25519-v1\0" || SHA512(prefix)` using
ordinary Ed25519; it is explicitly NOT Ed25519ph. Prefix is every byte from
offset zero through the first 64 bytes of the signature block. Thus all header,
YLSpec, TOC, padding, stored payload, key ID and algorithm bytes are authenticated.
Verification uses a host trust callback that supplies the public key and rejects
revoked/unknown keys. Cryptographic validity alone grants no publisher, permission
or native execution authorization. Unsigned is a distinct state. Hosts MUST
require successful verification when their policy requires signatures; inspection
and hashing alone do not authenticate a package.

## Publication and recovery

Writers build a new sibling temporary file and publish without replacing an
existing destination. Failed writes MUST leave no partially published package.
Truncated files MUST fail validation; there is no salvage-and-execute mode.
Builds using store/Zstd and no encryption/signature randomness are reproducible
for identical ordered metadata and resource inputs. Signatures are deterministic;
encrypted outputs intentionally are not.

## SDK evolution without a wire change

SDK 0.1.1 adds language frontends and examples but does not change any header,
record, algorithm ID or profile rule above. Host-language JSON is internal
argument/report transport only. Each frontend delegates container decoding,
validation, hashing, encryption and signatures to the same native implementation.
Binary conformance remains defined by this specification and published vectors.
