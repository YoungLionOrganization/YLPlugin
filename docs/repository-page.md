# GitHub repository page settings

The README contains the project's overview, package badges, examples, installation,
architecture, release evidence and LGPLv3 links. The separate GitHub **About**
description, website and topics need repository administration permission.
The current integration returned `403 Resource not accessible by integration`
for both repository metadata and topic updates. Its repository-content access
does not include GitHub administration operations, even when the account itself
is an administrator. Wikis have now been enabled by the owner; a first saved
Home page is needed before the Wiki Git repository can be initialized/accessed.

The configured sponsorship destination is Patreon `YoungLionOrganization` in
`.github/FUNDING.yml`; it is preserved and linked from the README/Wiki.

To apply the prepared settings, open the gear next to **About** on
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
Use the documentation index as the website until the Wiki pages are published.
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
