# Data catalog example

This Qt-free container carries `catalog.json`, a small dataset with total 42.
Its three host targets demonstrate a no-permission viewer, an analysis host with
an exact signed-64 permission mask and an explicitly opted-in generic fallback.
The SDK does not define the registry's bit meanings.

Inspect/validate `../artifacts/data.ylplugin`, or run `examples/build_examples.py`
from the repository root to rebuild into a new directory. `read.py` accepts the
native SDK path and package path; `read.mjs` uses the Node addon selected through
YLPLUGIN_NODE_PATH or a packaged prebuild.

See [all example commands](../../docs/examples.md) and the
[profile](../../docs/profile.md). This is a data resource, not executable Python
or JavaScript plugin code.
