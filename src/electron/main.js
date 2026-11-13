const { app, BrowserWindow } = require("electron");
const { spawn } = require("child_process");
const fs = require("fs");
const path = require("path");

let backendProcess = null;

function startBackend() {
  const root = path.resolve(__dirname, "../..");
  const python = process.env.MS2_PYTHON || "python3";
  const script = path.join(root, "src", "python", "backend", "server.py");
  const cli = process.env.MS2_CLI || path.join(root, "build", "ms2_cli");

  backendProcess = spawn(python, [script, "--port", "8765", "--cli", cli], {
    cwd: root,
    env: { ...process.env, MS2_CLI: cli },
    stdio: ["ignore", "pipe", "pipe"],
  });

  backendProcess.stdout.on("data", (data) => {
    process.stdout.write(`[backend] ${data}`);
  });
  backendProcess.stderr.on("data", (data) => {
    process.stderr.write(`[backend] ${data}`);
  });
  backendProcess.on("error", (error) => {
    console.error("Failed to start the MS2 backend:", error);
  });
}

function createWindow() {
  const window = new BrowserWindow({
    width: 1440,
    height: 920,
    minWidth: 1080,
    minHeight: 720,
    backgroundColor: "#0B0E1A",
    title: "MS2 Beam",
    webPreferences: {
      preload: path.join(__dirname, "preload.js"),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true,
    },
  });

  window.loadFile(path.join(__dirname, "renderer", "index.html"));

  if (process.env.MS2_DEV || process.env.MS2_SCREENSHOT) {
    window.webContents.on("console-message", (...args) => {
      console.log("[renderer]", ...args);
    });
  }

  window.webContents.on("did-fail-load", (event, code, description, url) => {
    console.error(`[renderer] failed to load ${url}: ${code} ${description}`);
  });

  if (process.env.MS2_DEV) {
    window.webContents.openDevTools({ mode: "detach" });
  }

  if (process.env.MS2_SCREENSHOT) {
    window.webContents.once("did-finish-load", async () => {
      await new Promise((resolve) => setTimeout(resolve, 4000));
      if (process.env.MS2_TEST_VIEW) {
        await window.webContents.executeJavaScript(
          `document.querySelector('button[data-view="${process.env.MS2_TEST_VIEW}"]')?.click();`
        );
        await new Promise((resolve) => setTimeout(resolve, 800));
      }
      const image = await window.webContents.capturePage();
      fs.writeFileSync(process.env.MS2_SCREENSHOT, image.toPNG());
      console.log(`[test] screenshot written to ${process.env.MS2_SCREENSHOT}`);
      if (process.env.MS2_EXIT_AFTER_SCREENSHOT) {
        app.quit();
      }
    });
  }
}

app.whenReady().then(() => {
  startBackend();
  createWindow();

  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

app.on("window-all-closed", () => {
  if (process.platform !== "darwin") {
    app.quit();
  }
});

app.on("before-quit", () => {
  if (backendProcess && !backendProcess.killed) {
    backendProcess.kill("SIGTERM");
    backendProcess = null;
  }
});
