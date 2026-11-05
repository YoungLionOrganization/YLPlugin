# SPDX-FileCopyrightText: 2026 Cavanshirpro
# SPDX-License-Identifier: LGPL-3.0-only

"""Exercise the actual executable and Python binding against the same core."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "bindings/python"))
from ylplugin import Library, Error

CLI, NATIVE = sys.argv[1:3]
checks = 0


def run(*args, expected=0):
    global checks
    result = subprocess.run([CLI, *map(str, args)], text=True, encoding="utf-8", capture_output=True)
    assert result.returncode == expected, (args, result.returncode, result.stdout, result.stderr)
    checks += 1
    return result


def rejected(fn, code):
    global checks
    try:
        fn()
    except Error as error:
        assert error.code == code, (error.code, str(error))
        checks += 1
    else:
        raise AssertionError("operation should have failed")


with tempfile.TemporaryDirectory(prefix="ylplugin-integration-") as temporary:
    work = Path(temporary).resolve()
    library = Library(NATIVE)
    manifest = (ROOT / "examples/plugin.ylspec").read_text()
    fixture = ROOT / "examples/artifacts/game.rcc"
    assert fixture.read_bytes().startswith(b"qres"), "real RCC artifact required"
    run("--version")
    run("--help")
    output = work / "converted.ylplugin"
    run("convert", fixture, "--manifest", ROOT / "examples/plugin.ylspec", "--output", output, "--json")
    run("validate", output, "--json")
    discovery = json.loads(run("metadata", output, "--json").stdout)
    assert discovery["scope"] == "metadata-only" and discovery["integrity_status"] == "UNVERIFIED"
    assert run("metadata", output).stdout == library.read_metadata(output)
    unicode_file = work / "Cavanşir-🦁.ylplugin"
    unicode_file.write_bytes(output.read_bytes())
    assert json.loads(run("metadata", unicode_file, "--json").stdout)["metadata"] == library.read_metadata(output)
    run("validate", unicode_file, "--json")
    inspection = json.loads(run("inspect", output, "--json").stdout)
    assert inspection["signed"] is False and inspection["profile_status"] == "VALID"
    assert json.loads(run("list", output, "--json").stdout)[0]["path"] == "game.rcc"
    run("check", output, "--host", ROOT / "examples/host.json", "--json")
    run("check", output, "--host", ROOT / "examples/host.json", "--target", "leominigames", "--json", expected=4)
    with library.open(output) as package:
        package.validate()
        assert package.read("game.rcc") == fixture.read_bytes()
        assert package.read_slice("game.rcc", 0, 4) == b"qres"
        assert len(package.targets) == 3 and len(package.entrypoints) == 2
        host = json.loads((ROOT / "examples/host.json").read_text())
        assert package.check(host)["status"] == "SUPPORTED"
        rejected(lambda: package.read("game.rcc", cancel=lambda: True), 10)
        if os.name != "nt":
            package.extract(work / "python-extracted")
            assert (work / "python-extracted/game.rcc").read_bytes() == fixture.read_bytes()
    if os.name != "nt":
        run("extract", output, "--output", work / "cli-extracted", "--json")
        run("extract", output, "--output", work / "cli-extracted", "--json", expected=3)
    key = os.urandom(32)
    key_path = work / "content.key"
    key_path.write_bytes(key)
    encrypted = work / "encrypted.ylplugin"
    run("encrypt", output, "--output", encrypted, "--key-id", "license", "--key-file", key_path, "--json")
    run("validate", encrypted, "--json", expected=6)
    run("validate", encrypted, "--key-id", "license", "--key-file", key_path, "--json")
    with library.open(encrypted) as package:
        assert package.metadata and package.inspect()["profile_status"] == "VALID"
        rejected(lambda: package.read("game.rcc", keys=lambda _: b"x" * 32), 6)
        package.validate(keys=lambda name: key if name == "license" else None)
        assert package.read("game.rcc", keys=lambda _: key) == fixture.read_bytes()
        assert package.check(host)["status"] == "NEEDS_LICENSE"
        assert package.check(host, keys=lambda _: key)["status"] == "SUPPORTED"
    private, public = work / "private.pem", work / "public.pem"
    subprocess.run(["openssl", "genpkey", "-algorithm", "ED25519", "-out", str(private)], check=True, capture_output=True)
    subprocess.run(["openssl", "pkey", "-in", str(private), "-pubout", "-out", str(public)], check=True, capture_output=True)
    signed = work / "signed.ylplugin"
    run("sign", encrypted, "--output", signed, "--key-id", "publisher", "--private-key", private, "--json")
    run("verify", signed, "--key-id", "publisher", "--public-key", public, "--json")
    run("verify", signed, "--key-id", "revoked", "--public-key", public, "--json", expected=5)
    run("encrypt", signed, "--output", work / "drop", "--key-id", "license", "--key-file", key_path, "--json", expected=3)
    # DER SubjectPublicKeyInfo's last 32 bytes are the Ed25519 raw public key.
    der = subprocess.run(["openssl", "pkey", "-pubin", "-in", str(public), "-outform", "DER"], check=True, capture_output=True).stdout
    with library.open(signed) as package:
        package.verify(lambda name: der[-32:] if name == "publisher" else None)
        rejected(lambda: package.verify(lambda _: None), 7)
    python_output = work / "python-built.ylplugin"
    library.create(python_output, manifest, [{"path": "game.rcc", "file": str(fixture), "media": "application/x-qt-resource"}])
    assert python_output.read_bytes() == output.read_bytes()
    library.sign(python_output, work / "python-signed", "publisher", private.read_bytes())
    run("verify", work / "python-signed", "--key-id", "publisher", "--public-key", public, "--json")
    resources = work / "resources"
    resources.mkdir()
    (resources / "game.rcc").write_bytes(fixture.read_bytes())
    run("create", "--root", resources, "--manifest", ROOT / "examples/plugin.ylspec", "--output", work / "root-built", "--compression", "zstd", "--json")
    assert (work / "root-built").read_bytes() == output.read_bytes(), "RCC must not be double-compressed"
    corrupt = work / "truncated"
    corrupt.write_bytes(output.read_bytes()[:127])
    assert json.loads(run("inspect", corrupt, "--json", expected=3).stderr)["code"] == 1
    run("nonsense", output, "--json", expected=9)
    run("create", "--nonsense", "bad", "--json", expected=3)
    assert not list(work.glob(".ylplugin-*")), "no staging leaks"
print(f"{checks} CLI/Python checks passed")
