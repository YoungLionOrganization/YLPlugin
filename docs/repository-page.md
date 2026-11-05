# GitHub repository page settings

The README contains the project's overview, package badges, examples, installation,
architecture, release evidence and LGPLv3 links. The separate GitHub **About**
description, website and topics need repository administration permission.
The current integration returned `403 Resource not accessible by integration`
for both repository metadata and topic updates. Its repository-content access
does not include GitHub administration operations, even when the account itself
is an administrator. The owner enabled Wikis and initialized Home; the detailed
English Wiki has now been published and its remote content verified.

The configured sponsorship destination is Patreon `YoungLionOrganization` in
`.github/FUNDING.yml`; it is preserved and linked from the README/Wiki.

## Verified current state

The owner applied the prepared About settings through GitHub's UI. Repository
readback confirmed the description below, the published Wiki website URL and all
15 proposed topics. The Wiki is enabled and its 23 Markdown files are published.
The managed integration's administration restriction remains a tool limitation,
not an outstanding repository setup task.

To maintain these settings later, open the gear next to **About** on
https://github.com/YoungLionOrganization/YLPlugin and save these values.

## Description

```text
Portable .ylplugin containers with a C++20 core and C, JavaScript/TypeScript, Java/Kotlin and Python SDKs. Signed resources, host preflight and optional Qt RCC support. LGPLv3.
```

## Website

```text
https://github.com/YoungLionOrganization/YLPlugin/wiki
```

## Topics

```text
ylplugin plugin-system plugin-framework binary-format cpp20 c javascript typescript java kotlin python qt6 ylspec lgplv3 native-sdk
```

These settings do not enable GitHub Pages or change repository visibility.
The Wiki is now available as the repository website destination.
Machine-readable settings are in [repository-settings.json](repository-settings.json)
and [repository-topics.json](repository-topics.json). With an independently
authorized administrator's GitHub CLI connection, these are equivalent to the
About UI changes:

```sh
gh api --method PATCH repos/YoungLionOrganization/YLPlugin --input docs/repository-settings.json
gh api --method PUT repos/YoungLionOrganization/YLPlugin/topics --input docs/repository-topics.json
```

The current managed integration cannot apply those administration calls. This
is separate from organization Actions secrets and does not require changing them.
