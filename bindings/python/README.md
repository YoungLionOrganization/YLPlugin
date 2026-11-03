# YLPlugin Python SDK

Native-backed containers for Python 3.11+: inspect public TOML, read verified
resources, create, validate, preflight, sign, verify and extract through one core.

```python
import json
from ylplugin import Library

with Library().open("data.ylplugin") as plugin:
    plugin.validate()
    print(json.loads(plugin.read("catalog.json"))["total"])
```

Version 0.1.2 packaging creates platform wheels with a runtime and dependency
notices. The published 0.1.0 wheel is facade-only; it needs an explicit SDK path.
Source builds can use `Library('/absolute/path/libylplugin.so')` or
`YLPLUGIN_LIBRARY`. No code is downloaded at import.

Integers remain integers, resources remain bytes, and host key/trust callbacks
are explicit. Use Limits and context managers. `Error.code` exposes native status;
bad sizes/NUL paths fail before ctypes. Callback close is prevented; operations
on one handle are serialized.

Documentation: https://github.com/YoungLionOrganization/YLPlugin/blob/main/docs/python-api.md

This is not a Python execution runtime or an OS sandbox. YLPlugin is LGPL-3.0-only.
License texts, project source archives and dependency notices ship with platform wheels.
See https://github.com/YoungLionOrganization/YLPlugin/blob/main/LICENSING.md.
