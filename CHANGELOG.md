# Changelog

## 0.1.3 — unreleased

### Fixed

* Node file opens reject unpaired UTF-16 surrogates instead of silently opening
  a filename containing the Unicode replacement character. Valid Unicode paths remain supported.
* Core file-open diagnostics use UTF-8 instead of the Windows local code page;
  opening/building valid Unicode paths no longer fails while preparing an error message.
* C++/Node/JVM slice reads honor cancellation on empty resources and after the
  final reader callback. Validation checks cancellation before metadata work.
* Existing CMake build directories reconfigure when `VERSION` changes, keeping
  native runtime versions synchronized with language package metadata.

### Performance

* Plaintext store validation hashes the complete resource in requests of at most
  1 MiB instead of allocating a resource-sized output buffer. Compressed/encrypted
  resources retain their existing decode/authentication path.
* Slice scratch buffers scale down for small resources and are empty for empty records.
* Added an isolated-process validation benchmark and regression coverage for full
  payload integrity, bounded reads, cancellation and Unicode path handling.

### Documentation

* Added performance, troubleshooting and contribution guides.
* Corrected source-install versus platform-wheel instructions and outdated
  publication/build statements. Published installation examples remain on 0.1.2.

## 0.1.2 — 2026-11-04

LGPL-3.0-only license texts, SPDX source notices, package metadata, source archives
and corresponding-source build instructions. Published on GitHub, npm, PyPI and Maven Central.

## 0.1.1 — 2026-11-04

Native JS/TS, Java/Kotlin and Python SDK packaging; data/WASM/native examples,
registry publication workflows and expanded documentation. File format/profile remain 1.0.

## 0.1.0

Initial experimental C/C++ core, C ABI, CLI, Python facade, signatures, encryption,
host preflight, native/data adapters and optional RCC support. See the historical release notes.
