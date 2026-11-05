# SPDX-FileCopyrightText: 2026 Cavanshirpro
# SPDX-License-Identifier: LGPL-3.0-only

"""Launch the bundled native CLI, also via ``python -m ylplugin``."""
import os
from pathlib import Path
import subprocess
import sys

from . import __version__


def main():
    executable = Path(os.environ.get("YLPLUGIN_CLI") or
                      Path(__file__).with_name("_native") / ("ylplugin.exe" if os.name == "nt" else "ylplugin"))
    executable = executable.resolve()
    if not executable.is_file():
        sys.exit("YLPlugin native CLI is missing. Install a supported platform wheel or set YLPLUGIN_CLI to your source-built executable.")
    actual = subprocess.check_output([str(executable), "--version"], text=True).strip()
    if actual != __version__:
        sys.exit(f"Native CLI {actual} does not match Python package {__version__}")
    if os.name == "nt":
        return subprocess.call([str(executable), *sys.argv[1:]])
    os.execv(str(executable), [str(executable), *sys.argv[1:]])


if __name__ == "__main__":
    sys.exit(main())
