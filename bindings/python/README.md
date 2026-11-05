# YLPlugin — native plugin containers for Python

[![PyPI](https://img.shields.io/pypi/v/ylplugin)](https://pypi.org/project/ylplugin/)
[![License: LGPLv3](https://img.shields.io/badge/license-LGPLv3-blue)](https://github.com/YoungLionOrganization/YLPlugin/blob/main/LICENSE)

Read public **YLSpec metadata directly**, inspect and build indexed `.ylplugin`
containers, verify resources and signatures, and check host compatibility before
activating a plugin. Python calls the shared native C/C++ core through a typed
ctypes API; metadata uses the original native YLSpec parser.

This README describes **0.1.3**. The standalone metadata APIs and bundled CLI are
new in that version; 0.1.2 platform wheels contain the SDK runtime only.

## Install the SDK, core and command together

```sh
python -m pip install ylplugin
ylplugin --version
python -m ylplugin --version
```

Supported platform wheels include both the native SDK runtime and the native
`ylplugin` executable. Pip registers the command in the active environment's
scripts directory. A virtual environment exposes it while activated; `--user`
installs require Python's user scripts directory on PATH. Administrator access,
a separate core download, an import-time network request and a C++ compiler are
not required for a supported wheel installation.

Wheels target Python 3.11+, Linux x86_64/ARM64 (glibc 2.39+), macOS x86_64/ARM64
(macOS 11+), and Windows x86_64. Source checkouts and unsupported systems require
a native build. Set `YLPLUGIN_LIBRARY` or pass `Library('/absolute/runtime/path')`
for a source-built SDK, and set `YLPLUGIN_CLI` for its executable. Native and
Python versions must match. Wheels contain their matching core; the Python
`ylspec` package is not a separate dependency.

## Discover plugin information with bounded reads

```python
import tomllib
from ylplugin import Library, Limits

sdk = Library()
metadata = sdk.read_metadata("package.ylplugin", Limits(metadata=1024 * 1024))
plugin_info = tomllib.loads(metadata)["data"]
print(plugin_info["id"], plugin_info["version"])
```

This reads **only the 128-byte container header and its YLSpec metadata frame**.
It does not construct a Package, scan the TOC, read resources, decrypt payloads
or read the signature region. A host-provided range source can use
`sdk.read_metadata_reader(size, reader, limits)`, where `reader(offset, count)`
returns exactly the requested bytes. No second Python format parser is involved.

```sh
ylplugin metadata package.ylplugin
ylplugin metadata package.ylplugin --json
```

Discovery returns canonical TOML and **unverified claims**. It does not establish
payload integrity, authentic publisher identity or safe execution. `--json`
labels its scope `metadata-only` and integrity `UNVERIFIED`. A file handle is
opened to read the requested ranges; the complete file is not read or loaded.

## Validate and read a resource

```python
import json
from ylplugin import Library

with Library().open("data.ylplugin") as plugin:
    plugin.validate()
    print(json.loads(plugin.read("catalog.json"))["total"])
    print(plugin.targets)
```

`Library.create/sign` and `Package.metadata/inspect/targets/entrypoints/read/
read_slice/validate/verify/check/extract` use the same core. `open` loads the
container index as well as metadata; use `read_metadata` for discovery alone.
Resource reads return bytes, metadata returns TOML, reports return dictionaries,
and permission integers remain exact Python integers. Use `Limits` and context
managers to control resources and deterministic handle cleanup.

Keys and trust providers are explicit host callbacks. `Error.code` exposes the
native status. Invalid ranges, bool limits and NUL paths fail before ctypes can
truncate them. A handle serializes operations and rejects callback close/reentry.
Signature verification and host preflight remain separate operations.

## CLI and documentation

The installed command launches the native CLI, with create, metadata, inspect,
list, validate, check, extract, convert, sign, verify and encrypt operations.
`ylplugin --help` lists their arguments. The SDK and CLI do not execute plugin
code during discovery, validation or preflight, and are not an OS sandbox.

- [Detailed English Wiki](https://github.com/YoungLionOrganization/YLPlugin/wiki)
- [Python API](https://github.com/YoungLionOrganization/YLPlugin/blob/main/docs/python-api.md)
- [Metadata-only reads](https://github.com/YoungLionOrganization/YLPlugin/blob/main/docs/metadata-only.md)
- [CLI reference](https://github.com/YoungLionOrganization/YLPlugin/blob/main/docs/cli.md)
- [Examples](https://github.com/YoungLionOrganization/YLPlugin/blob/main/docs/examples.md)
- [Issues](https://github.com/YoungLionOrganization/YLPlugin/issues) · [Changelog](https://github.com/YoungLionOrganization/YLPlugin/blob/main/CHANGELOG.md)

YLPlugin is experimental and licensed **LGPL-3.0-only**. License texts, project
source archives and dependency notices ship with platform wheels. Third-party
components retain their own licenses. See
[licensing and corresponding sources](https://github.com/YoungLionOrganization/YLPlugin/blob/main/LICENSING.md).
