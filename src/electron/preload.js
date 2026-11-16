// Preload script: exposes only read-only desktop metadata to the sandboxed
// renderer. No filesystem, shell or Node.js APIs are bridged here.
const { contextBridge } = require("electron");

contextBridge.exposeInMainWorld("ms2Desktop", {
  platform: process.platform,
  versions: {
    electron: process.versions.electron,
    chrome: process.versions.chrome,
    node: process.versions.node,
  },
});
