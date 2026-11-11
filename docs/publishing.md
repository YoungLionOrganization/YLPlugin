# Build, stage and publish libraries

The repository has three distinct operations. CI builds/tests without registry
credentials; the GitHub release workflow publishes verified core assets; the
registry workflow uploads previously built language distributions. A normal push
does not publish to npm, PyPI or Maven Central.

## Versions and identities

`VERSION` is authoritative and `tools/check_versions.py` verifies mirrored Python,
npm and Java metadata. Release 0.1.1 does not mutate v0.1.0. Configured names:

* npm: `@younglionorganization/ylplugin`
* PyPI: `ylplugin`
* Central: `xyz.younglion:ylplugin`, `ylplugin-kotlin`, `ylplugin-natives`

Registry presence is not asserted by configuration. First publication must have
ownership for these names. Version 0.1.2 and later use LGPL-3.0-only in npm,
Python metadata and Maven POMs. Packages include LGPLv3/GPLv3 texts and project
source archives. Third-party terms, including YLSpec's Apache license, remain separate.

## Organization secrets

Grant this repository access under organization Settings → Secrets and variables
→ Actions. Remove old repository copies; same-name repository secrets override
organization values, and environment values take precedence too.

| Secret | Use |
|---|---|
| NPM_TOKEN | npm package/scope publishing permission |
| PYPI_API_TOKEN | Permission to publish **ylplugin**, not only ylspec |
| MAVEN_CENTRAL_USERNAME / MAVEN_CENTRAL_PASSWORD | Central Portal generated user-token pair |
| MAVEN_GPG_PRIVATE_KEY / MAVEN_GPG_PASSPHRASE | OpenPGP signing, separate from portal authentication |

Run **Check inherited publishing secrets** to check names/availability only. It
does not show values, validate remote permissions or publish anything. Sharing
secret storage does not broaden a project-scoped token. Future OIDC/trusted
publisher setup can replace registry tokens after service-side configuration;
this workflow does not claim an unconfigured OIDC publisher already exists.

## Pipeline

1. **build-and-conformance** checks core/shared SDK, Qt tests, sanitizers and fuzzing.
2. **language-libraries** builds native Node/JNI/ctypes bundles on the declared
   desktop matrix, tests ESM/CJS/TS/Java/Kotlin/Python, builds platform wheels and
   collates third-party notices. Its package job emits `registry-distributions`:
   npm tarball, Python wheels, Central bundle, `libraries.json` and `SHA256SUMS`.
3. Publish the matching GitHub Release through **publish-verified-release** using
   a successful core run. The tag must refer to the same commit as the language
   artifacts. This release operation does not upload registry packages.
4. Run **publish-registry-libraries** manually on main with the language run ID
   and one registry choice. It checks successful main CI, published release/tag,
   commit, every checksum and package coordinates before exposing that registry's
   credentials to its upload step. It never rebuilds binaries at publication time.

The Central step signs an isolated bundle using temporary GNUPGHOME and passphrase
stdin, computes required checksums, submits to Central and polls deployment state.
No private key or token goes into artifacts/logs. npm lifecycle scripts are disabled.
PyPI uploads are noninteractive and metadata-checked. Credentials for a different
registry are not supplied to the active upload step.

Registry versions are immutable. After a partial upload inspect remote state
before retrying; a failed upload is not reported as success and this workflow
does not silently overwrite remote versions. Central validation errors retain a
deployment identifier for investigation. The initial implementation only built
distributions; registry publication was subsequently authorized and dispatched
for 0.1.1. Future versions still require an explicit publication action.

## Completed 0.1.2 publication

The owner-selected LGPL-3.0-only patch was published from commit
`81b1fa257ca7085f41180d2f2b08934a852c1940`. Core run `41709787677` and language
run `41709787678` passed. Publication succeeded in GitHub run `41711388362`,
npm run `41711646655`, PyPI run `41711655711` and Central run `41711661036`.
Public npm/PyPI metadata and all three Central POMs expose the selected LGPLv3 license.
This records completed uploads; subsequent versions still use the manual pipeline.

## Version 0.1.3 publication

The immutable release commit is `584ceb1c2e8cb1336eeb0c3a69259ee15d82a688`.
Its [core run](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/42384970996)
and [SDK run](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/42384970956)
passed all thirteen jobs. Publication uses those existing artifacts without rebuilding.

| Destination | Result | Workflow |
|---|---|---|
| [GitHub v0.1.3](https://github.com/YoungLionOrganization/YLPlugin/releases/tag/v0.1.3) | Published, five asset checksums verified | [42917423317](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/42917423317) |
| [PyPI 0.1.3](https://pypi.org/project/ylplugin/0.1.3/) | Published, five platform wheels; fresh-venv CLI/core smoke passed | [42917547121](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/42917547121) |
| Maven Central, three artifacts | Published; all three public POMs checked; published JNI/core and CLI smoke passed | [42917553273](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/42917553273) |
| npm 0.1.3 | Blocked: registry PUT returns E404; latest remains 0.1.2 | [42917539619](https://github.com/YoungLionOrganization/YLPlugin/actions/runs/42917539619) |

The npm package exists publicly. The configured token's write/package access
needs correction before another upload attempt. Update the organization NPM_TOKEN
for `@younglionorganization/ylplugin` without posting its value in issues/chat,
then rerun only that failed registry workflow. Do not re-upload the successful
PyPI/Maven versions or replace GitHub assets/tags. The failure is not reported as
completed npm publication.

The main branch continues as 0.1.4 development. Select v0.1.3 and registry version
0.1.3 for released core/CLI behavior; its source and published binaries stay immutable.
