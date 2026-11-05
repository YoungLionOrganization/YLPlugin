// SPDX-FileCopyrightText: 2026 Cavanshirpro
// SPDX-License-Identifier: LGPL-3.0-only

import {Package, create, readMetadata, type Host, type Permission} from '@younglionorganization/ylplugin';
const permission: Permission = {format: 'bitmask', registry: 'example.bits', registry_version: '1', value: 9223372036854775807n};
const host: Host = {id: 'example.host', api: '1.0', os: 'linux', arch: 'arm64', grants: {viewer: permission}};
const p = new Package('data.ylplugin');
const bytes: Buffer = p.read('data.json');
p.check(host);
p.close();
void bytes; void create;

const metadata: string = readMetadata("data.ylplugin", {metadata: 1048576n});
void metadata;
