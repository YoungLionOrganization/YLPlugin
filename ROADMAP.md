# YLPlugin roadmap

Status: living plan for the **0.1.3 release baseline**, **0.1.4 development** and the presentation
asset milestone. Later version numbers below are proposed milestones, not release
dates or claims that an API already exists. Container, profile and SDK versions
are independent.

The next major product goal is a **plugin front page without a whole-package
read**: discover the icon, README, license, contributing guide, security policy
and related documents from YLSpec, then fetch only the files the host requests.

See the [presentation asset architecture](docs/design/presentation-assets.md)
for the proposed catalog, byte layout, discovery rules, writer algorithm,
compatibility and acceptance criteria.

## 0.1.3 — release baseline

Implemented and verified:

- [x] Header + YLSpec-only metadata reads, including C/C++ and Python range readers.
- [x] Shared native operation exposed to Python, JS/TS and Java/Kotlin.
- [x] Bundled native core and CLI, Python console entry point, npm bin and JVM launcher.
- [x] Bounded-memory plaintext validation, cancellation fixes and Unicode path handling.
- [x] Expanded package metadata, English documentation and Wiki.
- [x] [Seven core CI jobs](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/42125664950)
  and [six SDK/package jobs](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/42125664789) passed.
- [x] Presentation asset architecture and a feasibility experiment using the current native writer.

Remaining release work:

- [x] Finalize 0.1.3 release notes and immutable source/artifact references.
- [x] Publish and verify GitHub, PyPI and Maven 0.1.3 distributions.
- [ ] Complete npm 0.1.3 publication after package token permissions are corrected; see the [publishing record](docs/publishing.md#version-013-publication).

The presentation catalog and selective asset APIs below are **planned**. They are
not part of the current 0.1.3 runtime. Keep `read_metadata` strictly metadata-only;
it must never start fetching an icon or document implicitly.

## Next 0.1.x milestone — presentation assets v1 (candidate: 0.1.4)

This milestone is the priority after the current baseline. Use an optional,
versioned YLSpec extension and the existing container 1.0 resource layout.

### P1. Catalog and compatibility contract

- [x] Specify `extensions."xyz.younglion.ylplugin.presentation"`, version `1.0`.
- [x] Define asset IDs, roles, primary selections, package paths, media types,
  formats, text encodings, absolute byte offsets, lengths and SHA-256 hashes.
- [x] Keep file contents outside metadata: no Markdown bodies, image bytes,
  base64/data URLs or embedded document strings in the catalog.
- [x] Separate legacy/missing catalogs, unsupported versions and malformed catalogs.
- [x] Add a proposed-extension schema and positive/negative conformance vectors
  without changing the existing normative profile 1.0 schema prematurely.

The [native parser and schema contract](docs/presentation-catalog.md) now implement
P1 on the 0.1.4 branch. File detection, locator generation and asset reads are
not implemented yet.

Acceptance: metadata alone describes declared front-page files; old packages
remain readable and optional presentation data does not change runtime permissions.

### P2. Deterministic writer and automatic detection

- [ ] Detect conventional files during CLI packaging inside an explicit project
  root; document YLPlugin's search precedence instead of claiming GitHub parity.
- [ ] Support icon, README, license, contributing, security, code of conduct,
  changelog and notice roles; retain LICENSE/COPYING companions where applicable.
- [ ] Support author overrides, role exclusions and disabling automatic detection.
- [ ] Snapshot selected inputs, hash their exact bytes and store them as public,
  uncompressed resources. Reuse a matching existing resource instead of duplicating it.
- [ ] Compute final catalog locations with a bounded monotone layout calculation.
- [ ] Recompute locators on every create/repack/encrypt operation; preserve public
  presentation assets when ordinary runtime resources are encrypted.
- [ ] Report missing explicit files, ambiguity, incompatible resource policies,
  size limits and generated-catalog conflicts before publishing output.

Acceptance: identical inputs produce identical unsigned/plaintext output, including
with mixed resource alignments; stale or caller-supplied offsets are never reused.

### P3. Selective reads in the native core

- [ ] Add a discovery handle that reads only the header and YLSpec frame.
- [ ] List descriptors without reading any asset body or TOC record.
- [ ] Read one declared asset by ID through local or custom range I/O, without
  constructing a `Package` or reading the TOC, unrelated payload or signature.
- [ ] Check source bounds, independent presentation budgets, cancellation and
  the complete selected asset hash before returning bytes.
- [ ] Add explicit catalog-to-TOC consistency checking for hosts choosing full validation.

Acceptance: a virtual 5 GiB source rejects every request outside metadata and the
selected asset; source access volume is independent of unrelated payload size.

### P4. CLI and all SDKs

- [ ] Add descriptor listing and explicit asset export commands.
- [ ] Expose the same native discovery/read operation through C/C++, Python,
  Node ESM/CJS + TypeScript declarations, Java and Kotlin.
- [ ] Preserve byte buffers and exact offsets; add separate versioned presentation
  options instead of changing the existing C limits structure in place.
- [ ] Test installed wheels, npm archives and JVM artifacts, not only source imports.

Acceptance: each SDK reads identical selected bytes, has deterministic ownership,
returns the same error categories and performs no import-time binary download.

### P5. Front-page example and release gate

- [ ] Add an English host example that lists roles, loads an icon on request and
  loads README/license/security text only when their views are opened.
- [ ] Keep Markdown rendering and image decoding in the host; disable executable
  content and implicit remote fetches in the example.
- [ ] Cover absent/empty files, hash tampering, invalid locators, mixed alignments,
  duplicate declarations, old-writer repacks and encrypted/signed package behavior.
- [ ] Document that checksum-verified preview bytes do not authenticate a publisher.
- [ ] Run the platform matrix and publish the feature only after P1–P5 pass.

## 0.2.x — richer presentation, after the basic contract stabilizes

- [ ] Localized documents and alternate icons with explicit locale/theme selection.
- [ ] Bounded, declared README image attachments and package-relative link resolution.
- [ ] Host examples for safe SVG/advanced markup policies without adding a renderer
  or network client to the native container core.
- [ ] A separate design for authenticating discovery metadata without scanning
  runtime payloads. The existing whole-prefix signature cannot provide that guarantee.
- [ ] Cache/snapshot integration examples for authenticated registries and HTTP ranges.

These items must preserve explicit reads: discovering metadata must not eagerly
load every preview asset or follow links across the package or network.

## Toward 1.0

- [ ] Freeze validated container/profile/extension contracts independently of SDK APIs.
- [ ] Establish compatibility and migration tests across released SDK versions.
- [ ] Stabilize public ownership, errors, cancellation and resource-budget interfaces.
- [ ] Complete release reproducibility and corresponding-source checks.
- [ ] Arrange an independent security review before making production assurance claims.

## Scope boundaries

A presentation catalog supports host-built front pages. It does not install a
plugin, infer legal permission from a filename, grant execution rights, replace
preflight, or make a package authentic by hashing its README. YLPkg installation
and runtime adapters remain separate concerns.
