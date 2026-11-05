#!/usr/bin/env node
// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only
'use strict';
const {resolve, join} = require('node:path');
const {spawnSync} = require('node:child_process');
const {version} = require('../package.json');
const executable = resolve(process.env.YLPLUGIN_CLI || join(__dirname, '..', 'prebuilds',
  `${process.platform}-${process.arch}`, process.platform === 'win32' ? 'ylplugin.exe' : 'ylplugin'));
const probe = spawnSync(executable, ['--version'], {encoding: 'utf8'});
if (probe.error || probe.status !== 0 || probe.stdout.trim() !== version) {
  console.error(`YLPlugin ${version} native CLI is missing or incompatible: ${executable}`);
  process.exit(1);
}
const result = spawnSync(executable, process.argv.slice(2), {stdio: 'inherit'});
if (result.error) { console.error(result.error.message); process.exit(1); }
if (result.signal) process.kill(process.pid, result.signal);
else process.exit(result.status ?? 1);
