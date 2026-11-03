# Conformance vectors

Normative layout and algorithm IDs are in specification.md; field/permission/
selection rules are in profile.md. The following actual generated files are
checked in under `tests/vectors`; `expected.json` includes their SHA256 values:

| File | Open | Validate |
|---|---|---|
| valid.ylplugin | OK | OK |
| truncated.ylplugin | INVALID_CONTAINER | Not reached |
| mandatory-feature.ylplugin | INVALID_CONTAINER | Not reached |
| offset-overflow.ylplugin | INVALID_CONTAINER | Not reached |
| path-traversal.ylplugin | INVALID_CONTAINER | Not reached |
| payload-tamper.ylplugin | OK (structural only) | INTEGRITY |
| unsupported-profile.ylplugin | OK (inert inspection) | UNSUPPORTED_PROFILE |

`manifest.ylspec` is the positive profile vector. Negative-byte, oversized-byte
and string-byte profile vectors MUST be rejected. `sample.bin` includes NUL and
non-UTF8 payload bytes to distinguish binary resources from TOML metadata.
`valid.hex` is the complete offset-labelled dump, not an illustrative made-up file.

Its first 32 bytes are:

```text
00000000  59 4c 50 4c 55 47 00 00 01 00 00 00 80 00 00 00
00000010  00 00 00 00 00 00 00 00 6c 03 00 00 00 00 00 00
```

At offset `00000080` the genuine YLSpec magic begins:

```text
59 4c 53 50 45 43 00 00 00 00 01 00 20 00 00 00
```

Use the checked-in full dump/expected JSON for exact generated lengths. The
32-byte YLSpec header precedes all variable-length TOML bytes; it is not a fixed
32-byte metadata document.

Regeneration (requires a built native SDK):

```sh
python tools/generate_vectors.py /absolute/path/to/libylplugin.so
python tests/conformance.py /absolute/path/to/libylplugin.so
```

The native mutation tests cover additional invalid headers, framing, TOC lengths,
overlaps, filename aliases/prefix collisions, corrupted payloads/Zstd, bombs,
encryption/tag/AAD tampering, signatures, revoked keys, permission limits,
multi-host selection, fallback, cancellation, concurrent reads, resource policy,
symlink ancestors, shared extraction parents and partial-file rejection. Real
RCC conversion bytes and mount/engine teardown are tested separately. Signature/
encryption fixtures are generated with fresh ephemeral keys in the test temporary
directory; no private encryption/signing keys are embedded in this repository.

## Language consistency

0.1.1 adds cross-frontend round trips for the data fixture, exact maximum integer
masks, encrypted/compressed reads, revoked keys, cancellation and callback-close
rejection. Java additionally checks supplementary Unicode and cleanup; JS tests
ESM/CJS and the host-supplied WASM example. These tests reuse native parsing and
do not count language wrappers as independent container implementations.
