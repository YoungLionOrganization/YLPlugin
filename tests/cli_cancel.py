# SPDX-FileCopyrightText: 2026 Cavanshirpro
# SPDX-License-Identifier: LGPL-3.0-only

"""POSIX integration: cancel a real writer after staging begins, before publication."""
from pathlib import Path
import json
import signal
import subprocess
import sys
import tempfile
import time

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="ylplugin-signal-") as temporary:
    work = Path(temporary)
    resources = work / "resources"
    resources.mkdir()
    (resources / "game.rcc").write_bytes((root / "examples/artifacts/game.rcc").read_bytes())
    # Sparse inputs give the writer enough work without allocating a 512 MiB fixture.
    for i in range(32):
        with (resources / f"a{i:02}.bin").open("wb") as file:
            file.truncate(16 * 1024 * 1024)
    output = work / "interrupted.ylplugin"
    child = subprocess.Popen(
        [sys.argv[1], "create", "--manifest", str(root / "examples/plugin.ylspec"),
         "--root", str(resources), "--output", str(output), "--json"],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
    )
    try:
        deadline = time.monotonic() + 10
        while not list(work.glob(".ylplugin-*")):
            assert child.poll() is None, "writer exited before cancellation"
            assert time.monotonic() < deadline, "writer did not create staging"
            time.sleep(0.001)
        child.send_signal(signal.SIGINT)
        stdout, stderr = child.communicate(timeout=10)
        assert child.returncode == 8, (child.returncode, stdout, stderr)
        assert json.loads(stderr)["exit_code"] == 8
        assert not output.exists(), "interrupted package was published"
        assert not list(work.glob(".ylplugin-*")), "cancelled staging directory leaked"
    finally:
        if child.poll() is None:
            child.kill()
            child.communicate()
print("SIGINT cancellation, exit code, JSON diagnostic and staging cleanup passed")
