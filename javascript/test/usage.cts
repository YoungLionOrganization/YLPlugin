import SDK = require('@younglionorganization/ylplugin');
const p: SDK.Package = new SDK.Package('data.ylplugin');
const permissions: SDK.Permission = ['host.permission'];
void permissions;
p.close();

const metadata: string = SDK.readMetadata("data.ylplugin", {metadata: 1048576});
void metadata;
