import {Package, create, type Host, type Permission} from '@younglionorganization/ylplugin';
const permission: Permission = {format: 'bitmask', registry: 'example.bits', registry_version: '1', value: 9223372036854775807n};
const host: Host = {id: 'example.host', api: '1.0', os: 'linux', arch: 'arm64', grants: {viewer: permission}};
const p = new Package('data.ylplugin');
const bytes: Buffer = p.read('data.json');
p.check(host);
p.close();
void bytes; void create;
