# SPDX-FileCopyrightText: 2026 Cavanshirpro
# SPDX-License-Identifier: LGPL-3.0-only

import json
import sys
from ylplugin import Library
with Library(sys.argv[1]).open(sys.argv[2]) as plugin:
    plugin.validate()
    print(json.loads(plugin.read("catalog.json"))["total"])
