# Frequently Asked Questions

**0.1.3 development additions:** [direct metadata-only reads](Metadata-Only-Reads.md)
request only the 128-byte header and YLSpec frame. TOC, payload and signature
regions are not read. Discovery returns unverified metadata; validation and host
preflight remain separate. Python/npm now bundle the native CLI alongside the
core; Java/Kotlin supply a Java CLI entry point. See
[core/CLI installation and PATH behavior](../installation-cli.md).
These additions are not available in the published 0.1.2 packages.

## Is this an archive format or a plugin manager?

It is its own indexed container and native host SDK. It is not ZIP and does not
install components, resolve dependencies or provide a marketplace/account system.
YLPkg is a separate future installer boundary.

## Why does it use YLSpec?

YLSpec supplies public framed metadata and a profile envelope. YLPlugin adds its
own byte layout, resources, crypto and host plugin model. YLSpec's SDK/browser/
Android features do not automatically become YLPlugin features.

## Does opening a package run its code?

No. Inspect, validation, signature verification and preflight are distinct from
activation. A native load can execute constructors, so the host must approve it
before the activation stage.

## Which release should I install?

Use published 0.1.2 packages. The main branch prepares 0.1.3; match native/facade
versions for source builds. File/profile 1.0 and C ABI version 1 are separate
version namespaces and do not waive the SDK loader version check.

## Is there a browser, Android or iOS distribution?

The current JS package targets Node, JVM artifacts target desktop JNI, and
platform packages cover the declared desktop matrix. No browser YLPlugin SDK,
Android AAR, iOS package or Windows ARM64 native package is claimed.

## Can a plugin define its own permission bit meanings?

It can declare names/masks under host-scoped representations, but authority and
meanings come from the host's registry/policy. Exact registry version support
and explicit grants are required. Required and optional declarations differ.

## Does a signature make native code safe?

It authenticates bytes under host-supplied trust, not code behavior. Hosts still
review the publisher, API/ABI and payload. Native code is not sandboxed. RCC
also needs independent host review before Qt parses it.

## Can encrypted metadata hide the publisher and entrypoints?

Metadata is public. Content encryption protects record payloads and uses
host-provided keys. There is no automatic entitlement, account or perfect DRM system.

## Why does a tiny slice scan all bytes?

The format uses a complete-resource SHA256, not a per-chunk authenticated tree.
The SDK authenticates the entire resource before returning the selected bytes.
Development 0.1.3 lowers scratch usage and improves cancellation without caching trust.

## Why does Windows SDK support not enable native plugin activation?

SDK loading and container operations are supported. Secure untrusted plugin
extraction is not implemented on Windows, so the native execution adapter is
not enabled there. Those are different platform responsibilities.

## How do I update or unload a plugin?

Use host-controlled inventory/session management. Stop the session and destroy
runtime consumers before cleanup. Create a fresh approved session for an update.
There is no generic live patcher, application rollback service or forced RCC free.

## Where should I continue?

Read [Getting Started](Getting-Started.md), [Troubleshooting](Troubleshooting.md)
and [the canonical FAQ](../faq.md). File a minimal reproducible GitHub issue with
versions, platform, error code and expected/actual behavior when the guides do
not explain a problem.
