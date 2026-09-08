"use strict";

// Recent Node.js versions reject direct spawn/spawnSync execution of .cmd files
// on Windows. generate_web_ui.js invokes npx.cmd for the pinned esbuild bundle,
// so adapt only that Windows case while leaving Linux/macOS behavior untouched.
if (process.platform === "win32") {
  const childProcess = require("child_process");
  const originalSpawnSync = childProcess.spawnSync;

  childProcess.spawnSync = function patchedSpawnSync(command, args, options) {
    if (typeof command === "string" && command.toLowerCase().endsWith(".cmd")) {
      return originalSpawnSync(command, args, { ...options, shell: true });
    }
    return originalSpawnSync(command, args, options);
  };
}
