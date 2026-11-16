// Electron main process: starts the local Python backend, opens the renderer
// window and shuts the backend down when the application quits.
const { app, BrowserWindow } = require("electron");
const { spawn } = require("child_process");
const fs = require("fs");
const path = require("path");

let backendProcess = null; // Child process running src/python/backend/server.py.

function startBackend() {
  // Resolve paths from this file so the app works from any working directory.
  const root = path.resolve(__dirname, "../..");
  const python = process.env.MS2_PYTHON || "python3";
  const script = path.join(root, "src", "python", "backend", "server.py");
  const cli = process.env.MS2_CLI || path.join(root, "build", "ms2_cli");

  // The backend receives the CLI path explicitly; MS2_CLI is also exported for
  // compatibility with the environment used by the solver wrapper.
  backendProcess = spawn(python, [script, "--port", "8765", "--cli", cli], {
    cwd: root,
    env: { ...process.env, MS2_CLI: cli },
    stdio: ["ignore", "pipe", "pipe"],
  });

  // Forward backend logs to the Electron terminal, prefixed for clarity.
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
  // The renderer is sandboxed and only receives the small API exposed by
  // preload.js; it never gets direct Node.js access.
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

  // Development/debug modes mirror renderer console output to the terminal.
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

  // Automated screenshot mode used by the UI checks: wait for the first solve,
  // optionally switch view, capture the page and exit when requested.
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
  // Start the backend before the window so the renderer's health poll succeeds.
  startBackend();
  createWindow();

  // macOS keeps the app alive after closing the last window; recreate it when
  // the dock icon is clicked.
  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

app.on("window-all-closed", () => {
  // Windows/Linux quit with the last window; macOS follows its own convention.
  if (process.platform !== "darwin") {
    app.quit();
  }
});

app.on("before-quit", () => {
  // Stop the Python server so no orphan process keeps port 8765 open.
  if (backendProcess && !backendProcess.killed) {
    backendProcess.kill("SIGTERM");
    backendProcess = null;
  }
});
