#!/usr/bin/env node
"use strict";

const fs = require("fs");
const path = require("path");
const zlib = require("zlib");

function parseArgs() {
  const args = process.argv.slice(2);
  let webDir = path.join(".pio", "web-ui");
  for (let i = 0; i < args.length; ++i) {
    if (args[i] === "--web-dir" && i + 1 < args.length) {
      webDir = args[++i];
    } else if (args[i] === "--help" || args[i] === "-h") {
      console.log("Usage: node tools/finalize_web_bootstrap.js [--web-dir DIR]");
      process.exit(0);
    } else {
      throw new Error(`Unknown argument: ${args[i]}`);
    }
  }
  return { webDir };
}

function requireFile(file) {
  if (!fs.existsSync(file) || !fs.statSync(file).isFile()) {
    throw new Error(`Required Web UI asset is missing: ${file}`);
  }
}

function settingsStyle() {
  return `<style id="cube-settings-style">
#cube-settings-button{position:fixed;right:max(8px,calc((100vw - 768px)/2 + 8px));top:32px;z-index:9000;width:42px;height:42px;border:0;border-radius:50%;background:transparent;color:#6b7280;padding:0;display:flex;align-items:center;justify-content:center;text-decoration:none;cursor:pointer;transition:background .15s,color .15s}
#cube-settings-button svg{width:27px;height:27px;display:block;fill:none;stroke:currentColor;stroke-width:1.8;stroke-linecap:round;stroke-linejoin:round}
#cube-settings-button:hover,#cube-settings-button:focus-visible{background:#f3f4f6;color:#374151;outline:none}
#cube-settings-overlay{position:fixed;inset:0;z-index:9100;background:#0008;display:none;align-items:flex-start;justify-content:center;padding:16px;overflow:auto}
#cube-settings-overlay:target,#cube-settings-overlay.open{display:flex}
#cube-settings-panel{width:min(680px,100%);margin:16px auto;background:#fff;color:#333;border-radius:12px;padding:18px;font:14px system-ui,sans-serif;box-shadow:0 12px 36px #0005}
#cube-settings-panel h2{font-size:22px;margin:0}#cube-settings-panel h3{font-size:17px;margin:22px 0 10px;border-bottom:1px solid #ddd;padding-bottom:6px}
#cube-settings-panel .cs-head{display:flex;justify-content:space-between;align-items:center;gap:12px}
#cube-settings-panel .cs-close{border:0;background:transparent;font-size:26px;line-height:1;padding:4px 8px;color:#555;text-decoration:none}
#cube-settings-panel .cs-row{display:grid;grid-template-columns:minmax(130px,1fr) minmax(110px,1.4fr) auto;gap:8px;align-items:center;margin:8px 0}
#cube-settings-panel .cs-row.wide{grid-template-columns:minmax(130px,1fr) minmax(170px,2fr) auto}
#cube-settings-panel input,#cube-settings-panel select{min-width:0;width:100%;box-sizing:border-box;border:1px solid #aaa;border-radius:6px;background:#fff;padding:9px;color:#222;font:14px system-ui,sans-serif}
#cube-settings-panel button.cs-action{border:1px solid #999;border-radius:6px;background:#f5f5f5;color:#222;padding:9px 12px;white-space:nowrap;font-weight:600}
#cube-settings-panel button.cs-danger{border-color:#b55;color:#922;background:#fff7f7}
#cube-settings-panel .cs-info{background:#f5f7f8;border-radius:7px;padding:10px;line-height:1.6;margin:8px 0}
#cube-settings-panel .cs-note{font-size:12px;color:#666;line-height:1.5;margin:6px 0}
#cube-settings-status{min-height:20px;margin-top:14px;padding:8px;border-radius:6px;background:#f5f5f5;white-space:pre-wrap}
#cube-settings-status.error{background:#fff0f0;color:#a00}
#cube-settings-status.ok{background:#effaf1;color:#175d28}
@media(max-width:767px){#cube-settings-button{right:8px;top:12px;width:38px;height:38px}#cube-settings-button svg{width:25px;height:25px}}
@media(max-width:520px){#cube-settings-panel{padding:14px}#cube-settings-panel .cs-row,#cube-settings-panel .cs-row.wide{grid-template-columns:1fr auto}#cube-settings-panel .cs-row label{grid-column:1/-1}}
</style>`;
}

function settingsMarkup() {
  const servoRows = [
    ["stand-correct", "Stand correct (deg)"],
    ["stand-turn", "Stand turn (deg)"],
    ["arm-pull", "Arm pull (deg)"],
    ["arm-hold", "Arm hold (deg)"],
    ["arm-release", "Arm release (deg)"],
    ["arm-ready", "Arm ready (deg)"],
    ["servo-sleep", "Servo sleep (ms)"],
    ["arm-x-sleep", "Arm X sleep (ms)"],
  ].map(([key, label]) => `<div class="cs-row"><label for="cs-${key}">${label}</label><input id="cs-${key}" type="number" inputmode="numeric"><button type="button" class="cs-action" data-servo-key="${key}">Save</button></div>`).join("\n");

  return `<a id="cube-settings-button" href="#cube-settings-overlay" role="button" aria-haspopup="dialog" aria-label="Settings" title="Settings"><svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12.22 2h-.44a2 2 0 0 0-2 2v.18a2 2 0 0 1-1 1.73l-.43.25a2 2 0 0 1-2 0l-.15-.08a2 2 0 0 0-2.73.73l-.22.38a2 2 0 0 0 .73 2.73l.15.09a2 2 0 0 1 1 1.74v.5a2 2 0 0 1-1 1.74l-.15.09a2 2 0 0 0-.73 2.73l.22.38a2 2 0 0 0 2.73.73l.15-.08a2 2 0 0 1 2 0l.43.25a2 2 0 0 1 1 1.73V20a2 2 0 0 0 2 2h.44a2 2 0 0 0 2-2v-.18a2 2 0 0 1 1-1.73l.43-.25a2 2 0 0 1 2 0l.15.08a2 2 0 0 0 2.73-.73l.22-.38a2 2 0 0 0-.73-2.73l-.15-.09a2 2 0 0 1-1-1.74v-.5a2 2 0 0 1 1-1.74l.15-.09a2 2 0 0 0 .73-2.73l-.22-.38a2 2 0 0 0-2.73-.73l-.15.08a2 2 0 0 1-2 0l-.43-.25a2 2 0 0 1-1-1.73V4a2 2 0 0 0-2-2z"/><circle cx="12" cy="12" r="3"/></svg></a>
<div id="cube-settings-overlay" role="dialog" aria-modal="true" aria-labelledby="cube-settings-title">
  <div id="cube-settings-panel">
    <div class="cs-head"><h2 id="cube-settings-title">Settings</h2><a id="cube-settings-close" class="cs-close" href="#" role="button" aria-label="Close">×</a></div>
    <h3>Wi-Fi</h3>
    <div class="cs-info">Configured: <strong id="cs-configured-mode">-</strong><br>Active: <strong id="cs-active-mode">-</strong> / <span id="cs-active-ip">-</span><br>Default AP: <span id="cs-ap-ssid">-</span></div>
    <div class="cs-row"><label for="cs-wifi-mode">Mode</label><select id="cs-wifi-mode"><option value="sta">Station</option><option value="ap">Access Point</option></select><button id="cs-save-mode" type="button" class="cs-action">Save</button></div>
    <div class="cs-row wide"><label for="cs-wifi-ssid">Station SSID</label><input id="cs-wifi-ssid" type="text" maxlength="32" autocomplete="off"><span></span></div>
    <div class="cs-row wide"><label for="cs-wifi-password">New password</label><input id="cs-wifi-password" type="password" maxlength="63" autocomplete="new-password" placeholder="8–63 chars"><span></span></div>
    <label class="cs-note"><input id="cs-open-network" type="checkbox" style="width:auto;margin-right:6px">Open network (save an empty password)</label>
    <div class="cs-note">Stored password is never displayed. Saving a Wi-Fi setting automatically reboots the NanoC6 so the change takes effect.</div>
    <div style="display:flex;gap:8px;flex-wrap:wrap;margin-top:10px"><button id="cs-save-wifi" type="button" class="cs-action">Save credentials</button><button id="cs-clear-wifi" type="button" class="cs-action cs-danger">Clear credentials</button></div>
    <h3>Servo</h3>
    <div class="cs-note">Each value is saved to NVS and becomes effective immediately. No reboot is required. Changes are rejected while a robot sequence is running. Timing range is 0–5000 ms.</div>
    ${servoRows}
    <div id="cube-settings-status" aria-live="polite"></div>
  </div>
</div>`;
}

function settingsScript() {
  const source = `(()=>{
const init=()=>{
const q=id=>document.getElementById(id),btn=q("cube-settings-button"),overlay=q("cube-settings-overlay"),close=q("cube-settings-close"),status=q("cube-settings-status");
if(!overlay||!status){console.error("Cube settings UI initialization failed: required elements missing");return}
const alignLauncher=()=>{if(!btn)return;const title=document.querySelector("[x-title] h1");if(!title)return;const tr=title.getBoundingClientRect(),br=btn.getBoundingClientRect();const top=Math.max(8,tr.top+(tr.height-br.height)/2);btn.style.left="auto";btn.style.right="";btn.style.top=top+"px"};
const setStatus=(text,type="")=>{status.textContent=text;status.className=type};
const notify=(type,message,description="")=>{try{const a=window.Alpine,t=a&&typeof a.store==="function"?a.store("toast"):null;if(t&&typeof t[type]==="function"){t[type](message,description);return}if(typeof window.toast==="function"){window.toast(message,{type:type==="error"?"danger":type,description,position:window.innerWidth<768?"bottom-center":"top-right"});return}}catch(e){}setStatus(description?message+"\\n"+description:message,type==="error"?"error":"ok")};
const api=async(params,method="POST")=>{const url=new URL("/api/settings",location.origin);if(params)Object.entries(params).forEach(([k,v])=>url.searchParams.set(k,String(v)));const r=await fetch(url,{method,cache:"no-store"});let data={};try{data=await r.json()}catch(e){}if(!r.ok)throw new Error(data.error||("HTTP "+r.status));return data};
const load=async(showStatus=true)=>{if(showStatus)setStatus("Loading...");try{const d=await api(null,"GET"),w=d.wifi,s=d.servo;q("cs-configured-mode").textContent=w.configuredMode;q("cs-active-mode").textContent=w.activeMode;q("cs-active-ip").textContent=w.activeIp;q("cs-ap-ssid").textContent=w.defaultApSsid;q("cs-wifi-mode").value=w.configuredMode;q("cs-wifi-ssid").value=w.stationSsid||"";["stand-correct","stand-turn","arm-pull","arm-hold","arm-release","arm-ready","servo-sleep","arm-x-sleep"].forEach(k=>{const el=q("cs-"+k);if(el)el.value=s[k]});if(showStatus)setStatus(w.stationCredentialsStored?"Settings loaded. Station credentials are stored (password hidden).":"Settings loaded. No station credentials are stored.","ok")}catch(e){setStatus("Load failed: "+e.message,"error");notify("error","Load failed",e.message)}};
window.cubeSettingsLoad=load;
const rebootAfterWifiSave=async(message)=>{notify("success","Saved!",message+" Rebooting...");setStatus(message+" Rebooting automatically...","ok");await new Promise(r=>setTimeout(r,350));try{await api({action:"reboot"})}catch(e){}};
if(btn)btn.addEventListener("click",()=>setTimeout(()=>load(),0));
if(close)close.addEventListener("click",()=>overlay.classList.remove("open"));
window.addEventListener("hashchange",()=>{if(location.hash==="#cube-settings-overlay")load()});
window.addEventListener("resize",alignLauncher,{passive:true});
document.addEventListener("alpine:initialized",()=>requestAnimationFrame(alignLauncher),{once:true});
if(document.fonts&&document.fonts.ready)document.fonts.ready.then(alignLauncher).catch(()=>{});
requestAnimationFrame(()=>{alignLauncher();requestAnimationFrame(alignLauncher)});setTimeout(alignLauncher,100);setTimeout(alignLauncher,500);
overlay.addEventListener("click",e=>{if(e.target===overlay){history.replaceState(null,"",location.pathname+location.search)}});
const saveMode=q("cs-save-mode");if(saveMode)saveMode.addEventListener("click",async()=>{try{await api({action:"wifi-mode",mode:q("cs-wifi-mode").value});await rebootAfterWifiSave("Wi-Fi mode saved.")}catch(e){setStatus("Save failed: "+e.message,"error");notify("error","Save failed",e.message)}});
const openNetwork=q("cs-open-network");if(openNetwork)openNetwork.addEventListener("change",e=>{const p=q("cs-wifi-password");if(!p)return;p.disabled=e.target.checked;if(e.target.checked)p.value=""});
const saveWifi=q("cs-save-wifi");if(saveWifi)saveWifi.addEventListener("click",async()=>{const ssid=q("cs-wifi-ssid").value.trim(),open=q("cs-open-network").checked,pass=open?"":q("cs-wifi-password").value;if(!ssid||ssid.length>32){setStatus("SSID must be 1–32 characters.","error");notify("error","Save failed","SSID must be 1–32 characters.");return}if(!open&&(pass.length<8||pass.length>63)){setStatus("Password must be 8–63 characters, or select Open network.","error");notify("error","Save failed","Password must be 8–63 characters, or select Open network.");return}try{await api({action:"wifi-set",ssid,password:pass});q("cs-wifi-password").value="";await rebootAfterWifiSave("Station credentials saved.")}catch(e){setStatus("Save failed: "+e.message,"error");notify("error","Save failed",e.message)}});
const clearWifi=q("cs-clear-wifi");if(clearWifi)clearWifi.addEventListener("click",async()=>{if(!confirm("Clear stored station credentials?"))return;try{await api({action:"wifi-clear"});q("cs-wifi-password").value="";await rebootAfterWifiSave("Station credentials cleared.")}catch(e){setStatus("Clear failed: "+e.message,"error");notify("error","Clear failed",e.message)}});
document.querySelectorAll("[data-servo-key]").forEach(b=>b.addEventListener("click",async()=>{const key=b.dataset.servoKey,input=q("cs-"+key),value=input?input.value:"";if(value===""||!Number.isInteger(Number(value))){setStatus("Enter an integer for "+key+".","error");notify("error","Save failed","Enter an integer for "+key+".");return}try{await api({action:"servo-set",key,value});setStatus(key+" saved: "+value+" (effective immediately)","ok");notify("success","Saved!",key+" = "+value);await load(false)}catch(e){setStatus("Servo save failed: "+e.message,"error");notify("error","Save failed",e.message)}}));
if(location.hash==="#cube-settings-overlay")load();
};
if(document.readyState==="loading")document.addEventListener("DOMContentLoaded",init,{once:true});else init();
})()`;
  // Parse the generated browser script during the host build so a syntax error
  // cannot silently ship as a non-responsive Settings control.
  new Function(source);
  return `<script id="cube-settings-script">${source}</script>`;
}

function main() {
  const { webDir } = parseArgs();
  const indexPath = path.join(webDir, "index.html");
  const appPath = path.join(webDir, "j", "app.js");
  const alpinePath = path.join(webDir, "j", "alpine.js");
  const manifestPath = path.join(webDir, "manifest.json");

  requireFile(indexPath);
  requireFile(appPath);
  requireFile(alpinePath);

  let html = fs.readFileSync(indexPath, "utf8");
  const appTag = '\t<script defer src="./j/app.js"></script>\n';
  const alpineTag = '\t<script defer src="./j/alpine.js"></script>\n';
  if (!html.includes(appTag) || !html.includes(alpineTag)) {
    throw new Error("Expected app/alpine script tags were not found in generated index.html");
  }
  html = html.replace(appTag, "").replace(alpineTag, "");

  const previousBootstrap = '\t<script>document.addEventListener("alpine:initialized",()=>{const l=document.getElementById("boot-loader"),a=document.getElementById("app-shell");if(l)l.remove();if(a)a.hidden=false})</script>';
  if (!html.includes(previousBootstrap)) {
    throw new Error("Expected loading bootstrap was not found in generated index.html");
  }

  const bootstrap = `\t<script>(()=>{const l=document.getElementById("boot-loader"),a=document.getElementById("app-shell"),m=l?l.lastElementChild:null,s=l?l.querySelector(".spinner"):null;let ready=false,failed=false;const fail=x=>{if(failed)return;failed=true;if(s)s.style.display="none";if(m)m.textContent="Loading failed";console.error("Cube Scrambler Web UI load failed:",x)};document.addEventListener("alpine:initialized",()=>{ready=true;if(l)l.remove();if(a)a.hidden=false},{once:true});const load=src=>new Promise((ok,ng)=>{const e=document.createElement("script");e.src=src;e.onload=ok;e.onerror=()=>ng(src);document.head.appendChild(e)});load("./j/app.js").then(()=>load("./j/alpine.js")).catch(fail);setTimeout(()=>{if(!ready)fail("initialization timeout")},15000)})()</script>`;
  html = html.replace(previousBootstrap, bootstrap);

  if (!html.includes("</head>") || !html.includes("</body>")) {
    throw new Error("Generated index.html is missing head/body terminators");
  }
  html = html.replace("</head>", settingsStyle() + "\n</head>");
  html = html.replace("</body>", settingsMarkup() + "\n" + settingsScript() + "\n</body>");
  if (!html.includes('href="#cube-settings-overlay"') ||
      !html.includes('id="cube-settings-script"') ||
      !html.includes('<svg viewBox="0 0 24 24"')) {
    throw new Error("Generated Settings launcher validation failed");
  }
  fs.writeFileSync(indexPath, html);

  const gzip = zlib.gzipSync(Buffer.from(html, "utf8"), {
    level: zlib.constants.Z_BEST_COMPRESSION,
  });
  fs.writeFileSync(`${indexPath}.gz`, gzip);

  if (fs.existsSync(manifestPath)) {
    const manifest = JSON.parse(fs.readFileSync(manifestPath, "utf8"));
    manifest.bootstrap = {
      sequentialScripts: ["j/app.js", "j/alpine.js"],
      visibleFailureState: true,
      initializationTimeoutMs: 15000,
    };
    manifest.settingsUi = {
      inline: true,
      endpoint: "/api/settings",
      wifi: ["mode", "station credentials", "clear credentials", "automatic reboot"],
      servo: ["stand-correct", "stand-turn", "arm-pull", "arm-hold", "arm-release", "arm-ready", "servo-sleep", "arm-x-sleep"],
      launcher: "SVG gear vertically aligned to title; horizontally pinned to content/screen right edge",
      toast: "Alpine toast store with window.toast fallback",
    };
    fs.writeFileSync(manifestPath, JSON.stringify(manifest, null, 2) + "\n");
    const manifestGzip = zlib.gzipSync(fs.readFileSync(manifestPath), {
      level: zlib.constants.Z_BEST_COMPRESSION,
    });
    fs.writeFileSync(`${manifestPath}.gz`, manifestGzip);
  }

  console.log(`Web bootstrap finalized: ${indexPath}`);
  console.log(`  index.html: ${Buffer.byteLength(html, "utf8")} bytes`);
  console.log(`  index.html.gz: ${gzip.length} bytes`);
  console.log(`  app.js: ${fs.statSync(appPath).size} bytes`);
  console.log(`  alpine.js: ${fs.statSync(alpinePath).size} bytes`);
  console.log("  settings UI: SVG gear at content/screen right edge, title-centered vertically, CSS-target open fallback, toast results, auto-reboot Wi-Fi");
}

try {
  main();
} catch (error) {
  console.error(error.stack || error.message || String(error));
  process.exit(1);
}