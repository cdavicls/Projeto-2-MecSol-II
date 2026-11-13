const { contextBridge } = require("electron");

contextBridge.exposeInMainWorld("ms2Desktop", {
  platform: process.platform,
  versions: {
    electron: process.versions.electron,
    chrome: process.versions.chrome,
    node: process.versions.node,
  },
});
