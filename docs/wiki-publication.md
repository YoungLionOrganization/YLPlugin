# Maintaining and publishing the English Wiki

The English Wiki sources live in [docs/wiki](wiki/Home.md). They are versioned
with the SDK documentation and can be read in the main repository before or
independently of GitHub Wiki publication. Home, topic pages, sidebar and footer
are maintained together; the normative format/profile documents remain canonical.

## Published Wiki

The [English Wiki is live](https://github.com/YoungLionOrganization/YLPlugin/wiki).
The first full publication contains 23 Markdown files (21 topic pages, sidebar
and footer), plus LGPL/GPL license texts and a source manifest. Wiki commit
`fbb592340946f11148f56edccb035dc09e80d816` was authored as Cavanshirpro from source
commit `187ff9d2f24db82f63bc1f2859fd47f140ec52a9`. A fresh remote clone verified
every page/license hash against the publication manifest.

## Check and preview

```sh
python tools/publish_wiki.py --check
python tests/test_wiki.py
python tools/publish_wiki.py --output build/wiki-preview
```

Source pages use repository-relative Markdown links so they work on GitHub's
normal file viewer. Rendering changes page links to GitHub Wiki URLs and source/
specification links to main-repository URLs. Code examples and external links
are preserved. Missing/outside-repository links and unclosed fences are rejected.
The export manifest records source/rendered page hashes.

## Publish through the Wiki Git repository

Enable Wikis under repository Settings → General → Features. GitHub may require
an initial Home page saved through its Wiki UI before `.wiki.git` is available.
The Wiki Git repository must be accessible with the current Git credentials.

Commit the sources, then run:

```sh
python tools/publish_wiki.py --publish
```

The helper clones only `YoungLionOrganization/YLPlugin.wiki.git`, updates the
managed pages, LGPL/GPL license texts and a source manifest, commits as Cavanshirpro and pushes without
force. Unrelated existing Wiki pages/assets are preserved. Repeated publication
with identical sources/manifest does not create an empty commit. It does not
ask for or store a personal token in repository files.

The GitHub integration's repository-content access does not automatically grant
administration access or access to the separate Wiki Git repository. A successful
main-repository push is not evidence that About settings or Wiki publication
worked. Confirm the published Wiki and remote commit after the command succeeds.

## Repository metadata and sponsorship

About/website/topics and enabling the Wiki require repository administration
access. Prepared About values are in [repository page settings](repository-page.md).
The homepage may point to the published Wiki after it exists. Funding destinations
belong to the owner and should be copied from the actual sponsorship configuration,
not invented from an unrelated account or an ambient browser tab.

## Version policy

Keep installation commands pinned to the release baseline, currently 0.1.3.
Label later development-only behavior explicitly. Update release status,
SDK/platform claims and the verification links together after a real release.
Wiki publication does not publish npm/PyPI/Maven packages or change any SDK tag.
