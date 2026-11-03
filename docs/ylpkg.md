# Future YLPkg interoperability contract

YLPlugin is one logical component, with public metadata, multiple host targets,
runtime variants, permissions, resources, preflight, signatures and encryption.
It can be used directly without any package manager.

YLPkg is a distinct future format for installation/update/removal of application
components, engines, API services, codecs and runtimes; multi-plugin distribution;
dependencies/conflicts/provides; rollback and maintenance. A market game may be a
YLPlugin; a new physics engine installed into a host may be a YLPkg component.
No YLPkg installer, resolver, privileged service or account system is implemented.

The low-level wire substrate is the fixed container header, indexed resource
records, codecs and cryptographic region in `specification.md`. It contains no
host, game, permission-bit or installer semantics. YLPkg MAY reuse these record,
compression, AEAD and signature algorithms with a DISTINCT magic and profile.
It MUST NOT label installation metadata as `xyz.younglion.plugin` or reinterpret
a YLPlugin permission as an install instruction. A future shared library can
factor the existing container/crypto modules without rewriting algorithms;
this release exposes the substrate through the plugin SDK, not a frozen separate
YLPkg ABI. This is an architectural boundary, not a claim of an implemented
generic YLPkg writer.

A YLPkg distribution MAY store complete `.ylplugin` files as opaque resources
with media type `application/vnd.younglion.ylplugin`. Their inner signatures and
metadata MUST remain byte-for-byte intact. An outer package signature authenticates
distribution, not inner publisher/native authority. Installers MUST independently
validate inner formats, apply host policy, scope caches and publish changes
transactionally. Installed runtime IDs/versions become explicit host preflight
registrations only after successful installation; no YLPlugin metadata triggers
automatic downloads or privileged component installation.

Use resource IDs/paths plus SHA256, plugin id/version, host id, runtime id/version,
codec/encryption algorithm IDs and key IDs as future interface inputs. Neither
format should duplicate permission meanings, identity/account records, trust
stores or payment logic. Changes to the shared record/crypto wire contract require
an explicit new container version/algorithm ID, never an undocumented flag.

## Registry package versus YLPkg

An npm/PyPI/Maven distribution installs a host SDK in a developer ecosystem.
It is neither a .ylplugin payload nor a .ylpkg component installer. The new
language publication workflows do not implement dependency resolution for
plugins, install host engines automatically or give package metadata authority.
