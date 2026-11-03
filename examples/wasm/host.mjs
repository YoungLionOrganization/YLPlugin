import {Package} from '../../javascript/src/index.mjs';
const plugin = new Package(process.argv[2]);
try {
  // This application supplies the WebAssembly runtime. The SDK preflight never
  // instantiates code and does not claim a built-in WASM runtime adapter.
  const result = plugin.check({id: 'xyz.example.node-wasm', api: '1.0', os: process.platform,
    arch: process.arch, runtimes: {wasm: '1.0'}, memory: 67108864, storage: 0});
  if (result.status !== 'SUPPORTED') throw new Error(JSON.stringify(result));
  const verifiedBytes = plugin.read('add.wasm');
  const {instance} = await WebAssembly.instantiate(verifiedBytes, {});
  console.log(instance.exports.add(20, 22));
} finally { plugin.close(); }
