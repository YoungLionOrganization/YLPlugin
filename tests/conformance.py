from pathlib import Path
import hashlib
import json
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "bindings/python"))
from ylplugin import Library, Error

library = Library(sys.argv[1])
vectors = ROOT / "tests/vectors"
for name, expected in json.loads((vectors / "expected.json").read_text()).items():
    path = vectors / name
    assert hashlib.sha256(path.read_bytes()).hexdigest() == expected["sha256"], name
    try:
        package = library.open(path)
    except Error as e:
        assert e.code == expected["open"], (name, e.code)
        continue
    assert expected["open"] == 0, name
    with package:
        try:
            package.validate()
        except Error as e:
            assert e.code == expected.get("validate"), (name, e.code)
        else:
            assert expected.get("validate", 0) == 0, name
with tempfile.TemporaryDirectory() as temporary:
    for path in vectors.glob("*.ylspec"):
        output = Path(temporary) / path.stem
        resources = [{"path": "sample.bin", "file": str(vectors / "sample.bin")}]
        try:
            library.create(output, path.read_text(), resources)
        except Error:
            assert path.name != "manifest.ylspec", path.name
        else:
            assert path.name == "manifest.ylspec", path.name
            assert output.read_bytes() == (vectors / "valid.ylplugin").read_bytes()
print("Published binary/profile conformance vectors passed")
