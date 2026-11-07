# Roadmap and presentation assets

The [repository roadmap](../../ROADMAP.md) tracks current and future work.
0.1.3 is in development; 0.1.2 remains the latest published release.

## Current baseline

0.1.3 implements native metadata-only reads and bundled core/CLI installation.
Its [metadata API](Metadata-Only-Reads.md) reads the header and YLSpec frame,
without scanning the TOC or loading payload resources. All thirteen core and
SDK/package CI jobs passed for that implementation.

## Next priority: a plugin front page

A planned optional catalog will describe:

- Icon and README.
- License files and companion terms.
- Contributing guide and security policy.
- Code of conduct, changelog and notices.

Packaging will detect conventional files inside a bounded project root, with
explicit overrides. Metadata will contain only presence declarations, formats,
package paths, byte locations, lengths and hashes. The file bytes will remain
ordinary resources outside metadata.

A separate proposed discovery handle will list descriptors without loading files.
An explicit asset request will read only the chosen range and verify its hash,
without constructing a Package or scanning TOC, unrelated payloads or signatures.
The host will decide when to load each document or icon for its front page.

## Architecture and delivery gates

Read the [architecture proposal](../design/presentation-assets.md) for the schema,
automatic detection policy, integer-offset planning, range I/O, limits and
compatibility. A feasibility experiment with the current native writer reached
stable addresses in three rounds and read a README using 2001 bytes of a 12340-byte
fixture, including metadata, without reading its TOC or other assets.

That experiment is evidence for the design, not a released asset API. The
candidate next milestone is 0.1.4, after catalog, writer, native API, all bindings
and installed-package tests pass. Later milestones consider localized content,
declared README images and a separate authenticated-discovery design.

Checksums do not authenticate a publisher. The current whole-prefix signature
still requires a complete signed-prefix read for verification. Markdown rendering
and image decoding remain host concerns.

No new presentation command or SDK method in this proposal is implemented in the
current 0.1.3 runtime. The [roadmap](../../ROADMAP.md) is the delivery source of truth.
