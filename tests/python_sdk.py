# SPDX-FileCopyrightText: 2026 Cavanshirpro
# SPDX-License-Identifier: LGPL-3.0-only

from pathlib import Path
import concurrent.futures
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / "bindings/python"))
from ylplugin import Library, Limits, Error

sdk = Library(sys.argv[1])
fixture = root / "examples/artifacts/data.ylplugin"
with sdk.open(fixture) as package:
    assert package.targets[1]["required_permissions"]["value"] == 2**63 - 1
    assert package.permission_bytes(0, optional=True) == bytes([37,8,255,0])
    for value in (-1, True, 1.5, 2**64):
        try:
            package.read_slice("catalog.json", value, 1)
        except ValueError:
            pass
        else:
            raise AssertionError(value)
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as executor:
        assert all(b'42' in data for data in executor.map(lambda _: package.read("catalog.json"), range(20)))
for path in (str(fixture) + "\0other",):
    try: sdk.open(path)
    except ValueError: pass
    else: raise AssertionError("NUL accepted")
try: sdk.open(fixture, Limits(record=True))
except ValueError: pass
else: raise AssertionError("bool limit accepted")
with sdk.open(root / "tests/vectors/unsupported-profile.ylplugin") as package:
    try: package.targets
    except Error as error: assert error.code == 2
    else: raise AssertionError("profile error hidden")
with tempfile.TemporaryDirectory() as temporary:
    output = Path(temporary) / "encrypted"
    key = b'k' * 32
    sdk.create(output, (root / "examples/data/plugin.ylspec").read_text(), [{"path": "catalog.json", "file": str(root / "examples/data/catalog.json"), "key_id": "content"}], keys=lambda _: key)
    with sdk.open(output) as package:
        def close_inside_callback(_):
            package.close()
            return key
        try: package.read("catalog.json", keys=close_inside_callback)
        except Error as error: assert error.code == 8
        else: raise AssertionError("callback closed the active handle")
        assert b'42' in package.read("catalog.json", keys=lambda _: key)
print("Python limits, bigint, threading and callback ownership checks passed")
