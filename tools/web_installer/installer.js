"use strict";

(() => {
  const BAUD_RATE = 115200;
  const COMMAND_TIMEOUT_MS = 6000;
  const encoder = new TextEncoder();
  const decoder = new TextDecoder();

  const byId = (id) => document.getElementById(id);
  const mode = byId("wifi-mode");
  const ssid = byId("wifi-ssid");
  const password = byId("wifi-password");
  const openNetwork = byId("open-network");
  const stationFields = byId("station-fields");
  const validation = byId("wifi-validation");
  const configureButton = byId("configure-button");
  const clearLogButton = byId("clear-log-button");
  const serialStatus = byId("serial-status");
  const serialLog = byId("serial-log");
  const finishStep = byId("finish-step");
  const finishMessage = byId("finish-message");
  const openDeviceLink = byId("open-device-link");

  let port = null;
  let reader = null;
  let writer = null;
  let readBuffer = "";
  let waiters = [];

  function setStatus(element, message, type = "") {
    element.textContent = message;
    element.className = `status${type ? ` ${type}` : ""}`;
  }

  function appendLog(message) {
    serialLog.classList.remove("hidden");
    serialLog.textContent += `${message}\n`;
    serialLog.scrollTop = serialLog.scrollHeight;
  }

  function utf8Length(value) {
    return encoder.encode(value).length;
  }

  function currentWifiSettings() {
    if (mode.value === "ap") {
      return { mode: "ap", ssid: "", password: "", open: false };
    }

    const stationSsid = ssid.value.trim();
    const isOpen = openNetwork.checked;
    const stationPassword = isOpen ? "" : password.value;

    if (!stationSsid) throw new Error("SSID is required for Station mode.");
    if (utf8Length(stationSsid) > 32) throw new Error("SSID must be at most 32 bytes.");
    if (stationSsid.includes("|") || /[\r\n]/.test(stationSsid)) {
      throw new Error("SSID cannot contain | or line breaks in the USB provisioning flow.");
    }
    if (!isOpen) {
      const passwordBytes = utf8Length(stationPassword);
      if (passwordBytes < 8 || passwordBytes > 63) {
        throw new Error("Password must be 8–63 bytes, or select Open network.");
      }
      if (/[\r\n]/.test(stationPassword)) {
        throw new Error("Password cannot contain line breaks.");
      }
    }

    return {
      mode: "sta",
      ssid: stationSsid,
      password: stationPassword,
      open: isOpen,
    };
  }

  function validateWifiSettings() {
    try {
      const settings = currentWifiSettings();
      if (settings.mode === "ap") {
        setStatus(validation, "Access Point mode will be enabled after flashing.", "ok");
      } else {
        setStatus(validation, `Station mode is ready for SSID “${settings.ssid}”.`, "ok");
      }
      configureButton.disabled = !("serial" in navigator);
      return true;
    } catch (error) {
      setStatus(validation, error.message, "error");
      configureButton.disabled = true;
      return false;
    }
  }

  function updateModeUi() {
    const isStation = mode.value === "sta";
    stationFields.classList.toggle("hidden", !isStation);
    validateWifiSettings();
  }

  function updateOpenNetworkUi() {
    password.disabled = openNetwork.checked;
    if (openNetwork.checked) password.value = "";
    validateWifiSettings();
  }

  function resolveWaiters(line) {
    const remaining = [];
    for (const waiter of waiters) {
      if (waiter.match(line)) {
        clearTimeout(waiter.timer);
        waiter.resolve(line);
      } else {
        remaining.push(waiter);
      }
    }
    waiters = remaining;
  }

  function processText(text) {
    readBuffer += text;
    while (true) {
      const newline = readBuffer.indexOf("\n");
      if (newline < 0) break;
      const line = readBuffer.slice(0, newline).replace(/\r$/, "");
      readBuffer = readBuffer.slice(newline + 1);
      if (!line) continue;
      appendLog(line);
      resolveWaiters(line);
    }
  }

  async function readSerialLoop() {
    try {
      while (port && port.readable) {
        reader = port.readable.getReader();
        try {
          while (true) {
            const { value, done } = await reader.read();
            if (done) break;
            if (value) processText(decoder.decode(value, { stream: true }));
          }
        } finally {
          reader.releaseLock();
          reader = null;
        }
      }
    } catch (error) {
      if (port) appendLog(`[serial closed: ${error.message}]`);
    }
  }

  function waitForLine(match, timeoutMs = COMMAND_TIMEOUT_MS) {
    return new Promise((resolve, reject) => {
      const waiter = {
        match,
        resolve,
        reject,
        timer: setTimeout(() => {
          waiters = waiters.filter((item) => item !== waiter);
          reject(new Error("Timed out waiting for the NanoC6 response."));
        }, timeoutMs),
      };
      waiters.push(waiter);
    });
  }

  async function sendAndExpect(command, displayCommand, match) {
    if (!writer) throw new Error("Serial port is not writable.");
    appendLog(`> ${displayCommand}`);
    const response = waitForLine(match);
    await writer.write(encoder.encode(`${command}\n`));
    return response;
  }

  async function sendWithoutWaiting(command, displayCommand) {
    if (!writer) throw new Error("Serial port is not writable.");
    appendLog(`> ${displayCommand}`);
    await writer.write(encoder.encode(`${command}\n`));
  }

  async function openSerial() {
    port = await navigator.serial.requestPort();
    await port.open({ baudRate: BAUD_RATE });
    writer = port.writable.getWriter();
    void readSerialLoop();

    // Give the freshly flashed firmware time to complete a USB/boot transition.
    await new Promise((resolve) => setTimeout(resolve, 1200));
  }

  async function closeSerial() {
    for (const waiter of waiters) {
      clearTimeout(waiter.timer);
      waiter.reject(new Error("Serial connection closed."));
    }
    waiters = [];

    if (reader) {
      try { await reader.cancel(); } catch (_) {}
    }
    if (writer) {
      try { writer.releaseLock(); } catch (_) {}
      writer = null;
    }
    if (port) {
      try { await port.close(); } catch (_) {}
      port = null;
    }
  }

  function showFinish(settings) {
    if (settings.mode === "ap") {
      finishMessage.textContent = "Wi-Fi configuration was saved and the NanoC6 rebooted in Access Point mode. Connect to the CubeScrambler access point, then open 192.168.4.1.";
      openDeviceLink.href = "https://192.168.4.1/";
    } else {
      finishMessage.textContent = `Wi-Fi configuration for “${settings.ssid}” was saved and the NanoC6 rebooted in Station mode. If Station connection fails, the firmware automatically falls back to Access Point mode.`;
      openDeviceLink.href = "https://cube-scrambler.local/";
    }
    finishStep.classList.remove("hidden");
    finishStep.scrollIntoView({ behavior: "smooth", block: "start" });
  }

  async function configureWifi() {
    if (!("serial" in navigator)) {
      setStatus(serialStatus, "Web Serial is not available in this browser.", "error");
      return;
    }

    let settings;
    try {
      settings = currentWifiSettings();
    } catch (error) {
      setStatus(serialStatus, error.message, "error");
      return;
    }

    configureButton.disabled = true;
    finishStep.classList.add("hidden");
    serialLog.textContent = "";
    serialLog.classList.remove("hidden");
    setStatus(serialStatus, "Select the NanoC6 serial port. Waiting for USB connection…");

    try {
      await openSerial();
      setStatus(serialStatus, "NanoC6 connected. Applying Wi-Fi settings…");

      if (settings.mode === "sta") {
        await sendAndExpect(
          `wifi-set ${settings.ssid}|${settings.password}`,
          `wifi-set ${settings.ssid}|<hidden>`,
          (line) => line.includes("Station credentials save: PASS")
        );
        await sendAndExpect(
          "wifi-mode sta",
          "wifi-mode sta",
          (line) => line.includes("Wi-Fi mode save: PASS (sta)")
        );
      } else {
        await sendAndExpect(
          "wifi-mode ap",
          "wifi-mode ap",
          (line) => line.includes("Wi-Fi mode save: PASS (ap)")
        );
      }

      setStatus(serialStatus, "Wi-Fi settings saved. Rebooting NanoC6…", "ok");
      await sendWithoutWaiting("reboot", "reboot");
      await new Promise((resolve) => setTimeout(resolve, 350));

      password.value = "";
      setStatus(serialStatus, "Wi-Fi configuration complete.", "ok");
      showFinish(settings);
    } catch (error) {
      setStatus(
        serialStatus,
        `Wi-Fi configuration failed: ${error.message}\nClose the ESP Web Tools dialog, make sure the flashed firmware is running, then retry.`,
        "error"
      );
    } finally {
      await closeSerial();
      configureButton.disabled = false;
      validateWifiSettings();
    }
  }

  mode.addEventListener("change", updateModeUi);
  ssid.addEventListener("input", validateWifiSettings);
  password.addEventListener("input", validateWifiSettings);
  openNetwork.addEventListener("change", updateOpenNetworkUi);
  configureButton.addEventListener("click", configureWifi);
  clearLogButton.addEventListener("click", () => {
    serialLog.textContent = "";
    serialLog.classList.add("hidden");
  });

  if (!("serial" in navigator)) {
    configureButton.disabled = true;
    setStatus(serialStatus, "Web Serial is not available. Use a Chromium-based desktop browser.", "error");
  }

  updateModeUi();
  updateOpenNetworkUi();
})();
