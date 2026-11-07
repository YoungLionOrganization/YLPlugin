# Publishing and Maintenance

This guide describes the 0.1.3 release pipeline. Actual publication results are
recorded in the [publishing record](../publishing.md). A normal push builds/tests;
it does not perform registry publication.

## Identities and immutable versions

| Destination | Identity |
|---|---|
| GitHub | YoungLionOrganization/YLPlugin, tags vX.Y.Z |
| npm | @younglionorganization/ylplugin |
| PyPI | ylplugin |
| Maven Central | xyz.younglion:ylplugin, ylplugin-kotlin, ylplugin-natives |

VERSION is authoritative; mirrored Python/npm/Java values are checked. Container
and profile versions do not follow every SDK patch. Existing tags/assets/registry
files remain immutable. LGPL licensing was carried in 0.1.2 files rather than
rewriting historical 0.1.1 metadata.

## Verification and publication sequence

1. Commit a clean tree with correct versions, notes, licenses and sources.
2. Complete core and language CI for the same implementation commit.
3. Run the verified GitHub release workflow with that core run ID and matching tag.
4. Run registry publication with the successful language run ID and one registry.
5. Check the workflows and public metadata; inspect remote state before retrying
   any partial upload.

The release workflow verifies successful same-repository main CI, commit,
manifest and every checksum before publishing core assets. The registry workflow
checks matching release/tag, CI provenance, checksum inventory and package
identities before exposing selected credentials. It uploads existing tested
bytes; it does not rebuild binaries during publication.

The core release's universal Python wheel is facade-only. Language pipeline
platform wheels supply native runtimes. Distinguish those assets when writing
installation instructions.

## Organization secrets

The workflows use ordinary `secrets.NAME` references, which inherit authorized
organization secrets. Repository copies with the same name override organization
values; environment values take precedence too. Secret presence checks reveal
availability, not remote ownership or package permission scope.

| Secret | Purpose |
|---|---|
| NPM_TOKEN | npm package/scope upload |
| PYPI_API_TOKEN | PyPI ylplugin upload |
| MAVEN_CENTRAL_USERNAME/PASSWORD | Central Portal token pair |
| MAVEN_GPG_PRIVATE_KEY/PASSPHRASE | OpenPGP artifact signing |

A token limited to another project does not gain YLPlugin access by being stored
at organization level. Central signing and portal authentication are separate.
No compilation/test operation needs publishing credentials.

## Wiki, About and support links

Wiki pages are maintained as versioned English sources under docs/wiki. The
publication helper renders links and updates managed pages in the initialized
Wiki repository. About/website/topics and enabling Wiki require repository
administration access; content-write access alone may be insufficient.
Funding links should use the repository owner's configured destinations.

See [the publication guide](../publishing.md),
[Wiki maintenance](../wiki-publication.md) and [repository settings](../repository-page.md).
