"""Regenerate public, non-secret conformance vectors using the native writer."""
from pathlib import Path
import hashlib
import json
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "bindings/python"))
from ylplugin import Library

library = Library(sys.argv[1])
destination = ROOT / "tests/vectors"
destination.mkdir(exist_ok=True)
manifest = '''[_ylspec]
version = "0.1"
profile = "xyz.younglion.plugin"
profile_version = "1.0"
[data]
id = "xyz.test.vector"
version = "1.0"
[[data.entrypoints]]
id = "main"
runtime = "data"
runtime_min = "1.0"
runtime_max = "1.0"
resource = "sample.bin"
[[data.targets]]
id = "test"
host = "xyz.test.host"
api_min = "1.0"
api_max = "1.0"
entrypoint = "main"
required_permissions = { format = "bytes", registry = "test.bits", registry_version = "1", values = [37, 8, 255, 0] }
'''
(destination / "manifest.ylspec").write_text(manifest)
(destination / "sample.bin").write_bytes(b"YLPlugin\x00reference\xff\n")
valid = destination / "valid.ylplugin"
valid.unlink(missing_ok=True)
library.create(valid, manifest, [{"path": "sample.bin", "file": str(destination / "sample.bin")}])
original = valid.read_bytes()
vectors = {"valid.ylplugin": {"open": 0, "validate": 0}}


def vector(name, contents, open_status, validate_status=None):
    (destination / name).write_bytes(contents)
    vectors[name] = {"open": open_status}
    if validate_status is not None:
        vectors[name]["validate"] = validate_status


vector("truncated.ylplugin", original[:127], 1)
bad = bytearray(original)
struct.pack_into("<Q", bad, 16, 1 << 63)
vector("mandatory-feature.ylplugin", bad, 1)
bad = bytearray(original)
struct.pack_into("<Q", bad, 48, 2**64 - 8)
vector("offset-overflow.ylplugin", bad, 1)
bad = bytearray(original)
toc = struct.unpack_from("<Q", bad, 48)[0]
bad[toc + 128:toc + 138] = b"../ple.bin"
vector("path-traversal.ylplugin", bad, 1)
bad = bytearray(original)
bad[-1] ^= 1
vector("payload-tamper.ylplugin", bad, 0, 6)
bad = original.replace(b"xyz.younglion.plugin", b"xyz.younglion.plugix")
vector("unsupported-profile.ylplugin", bad, 0, 2)
for name, specification in vectors.items():
    specification["sha256"] = hashlib.sha256((destination / name).read_bytes()).hexdigest()
(destination / "expected.json").write_text(json.dumps(vectors, indent=2) + "\n")
dump = "\n".join(f"{off:08x}  {' '.join(f'{b:02x}' for b in original[off:off+16])}" for off in range(0, len(original), 16))
(destination / "valid.hex").write_text(dump + "\n")
for name, value in {"negative-byte": "[-1]", "oversized-byte": "[256]", "string-byte": '["37"]'}.items():
    (destination / f"{name}.ylspec").write_text(manifest.replace("[37, 8, 255, 0]", value))
print(f"Generated {len(vectors)} binary vectors and profile vectors")
