# SPDX-FileCopyrightText: 2026 Cavanshirpro
# SPDX-License-Identifier: LGPL-3.0-only

"""Check source and distribution license declarations without registry access."""
from pathlib import Path
import json
import tomllib

root = Path(__file__).resolve().parents[1]
assert 'GNU LESSER GENERAL PUBLIC LICENSE' in (root/'LICENSE').read_text()
assert 'Version 3, 29 June 2007' in (root/'LICENSE').read_text()
assert 'GNU GENERAL PUBLIC LICENSE' in (root/'COPYING').read_text()
assert 'Version 3, 29 June 2007' in (root/'COPYING').read_text()
project = tomllib.loads((root/'pyproject.toml').read_text())['project']
assert project['license'] == 'LGPL-3.0-only'
assert {'LICENSE', 'COPYING', 'SOURCES.md'}.issubset(project['license-files'])
package = json.loads((root/'javascript/package.json').read_text())
assert package['license'] == 'LGPL-3.0-only'
assert {'LICENSE', 'COPYING', 'SOURCES.md'}.issubset(package['files'])
assert json.loads((root/'javascript/package-lock.json').read_text())['packages']['']['license'] == package['license']
assert 'GNU Lesser General Public License, version 3.0 only' in (root/'jvm/build.gradle.kts').read_text()
assert 'LGPL-3.0-only' in (root/'LICENSING.md').read_text()
print('LGPLv3 texts and Python/npm/Maven license declarations match')
