// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

import {Package} from '../../javascript/src/index.mjs';
const plugin = new Package(process.argv[2]);
try { plugin.validate(); console.log(JSON.parse(plugin.read('catalog.json')).total); }
finally { plugin.close(); }
