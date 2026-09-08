#!/usr/bin/env node
"use strict";

const fs = require("fs");
const path = require("path");
const https = require("https");
const zlib = require("zlib");
const { spawnSync } = require("child_process");

const SOURCE_REPO = "miso-develop/cube-scrambler";
const SOURCE_REVISION = "ff14a5e9e1b721f7f009bd73945e4e5e4b3d4e51";
const DEFAULT_OUTPUT = path.join(".pio", "web-ui");
const ESBUILD_VERSION = "0.25.9";
const GZIP_EXTENSIONS = new Set([".html", ".css", ".js", ".json", ".svg"]);

// ESP32 SPIFFS uses a short object-name limit. Keep source paths for provenance,
// but flatten deployment paths so every generated object remains comfortably
// below the limit. JS module imports are patched before bundling below.
const ASSETS = [
  { source: "index.html", target: "index.html" },
  { source: "favicon.ico", target: "favicon.ico" },
  { source: "css/output.css", target: "c/style.css" },
  { source: "images/title-logo.png", target: "i/logo.png" },
  { source: "js/cube-cam-scanner.js", target: "j/cam.js" },
  { source: "js/lib/alpinejs@3.12.3.js", target: "j/alpine.js" },
  { source: "js/alpine/main.js", target: "j/main.js" },
  { source: "js/alpine/stores.js", target: "j/stores.js" },
  { source: "js/alpine/tabs.js", target: "j/tabs.js" },
  { source: "js/alpine/title.js", target: "j/title.js" },
  { source: "js/alpine/cubeUi/colorRing.js", target: "j/colorRing.js" },
  { source: "js/alpine/cubeUi/cubeFace.js", target: "j/cubeFace.js" },
  { source: "js/alpine/cubeUi/cubeMiniFace.js", target: "j/cubeMiniFace.js" },
  { source: "js/alpine/cubeUi/cubeUi.js", target: "j/cubeUi.js" },
  { source: "js/alpine/cubeUi/cubeUiSp.js", target: "j/cubeUiSp.js" },
  { source: "js/alpine/parts/button.js", target: "j/button.js" },
  { source: "js/alpine/parts/textInput.js", target: "j/textInput.js" },
  { source: "js/alpine/parts/toastNotification.js", target: "j/toast.js" },
];

function parseArgs() {
  const args = process.argv.slice(2);
  let output = DEFAULT_OUTPUT;
  for (let i = 0; i < args.length; ++i) {
    if (args[i] === "--output" && i + 1 < args.length) {
      output = args[++i];
    } else if (args[i] === "--help" || args[i] === "-h") {
      console.log("Usage: node tools/generate_web_ui.js [--output DIR]");
      process.exit(0);
    } else {
      throw new Error(`Unknown argument: ${args[i]}`);
    }
  }
  return { output };
}

function download(url, redirects = 0) {
  return new Promise((resolve, reject) => {
    https.get(url, { headers: { "User-Agent": "cube-scrambler-nanoc6" } }, response => {
      if (response.statusCode >= 300 && response.statusCode < 400 && response.headers.location) {
        response.resume();
        if (redirects >= 5) return reject(new Error(`Too many redirects: ${url}`));
        return resolve(download(response.headers.location, redirects + 1));
      }
      if (response.statusCode !== 200) {
        response.resume();
        return reject(new Error(`HTTP ${response.statusCode}: ${url}`));
      }
      const chunks = [];
      response.on("data", chunk => chunks.push(chunk));
      response.on("end", () => resolve(Buffer.concat(chunks)));
    }).on("error", reject);
  });
}

function replaceAll(source, replacements) {
  for (const [from, to] of replacements) {
    if (!source.includes(from)) {
      throw new Error(`Expected Web UI reference not found: ${from}`);
    }
    source = source.split(from).join(to);
  }
  return source;
}

function patchIndex(buffer) {
  let source = buffer.toString("utf8");
  source = replaceAll(source, [
    ["./css/output.css", "./c/style.css"],
    ['<script defer type="module" src="./js/alpine/main.js"></script>', '<script defer src="./j/app.js"></script>'],
    ["./js/lib/alpinejs@3.12.3.js", "./j/alpine.js"],
  ]);

  const loaderStyle = `\n\t<style>\n\t\t#boot-loader{position:fixed;inset:0;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:14px;background:#fff;color:#333;font:14px system-ui,sans-serif;z-index:9999}\n\t\t#boot-loader .spinner{width:32px;height:32px;border:4px solid #ddd;border-top-color:#555;border-radius:50%;animation:boot-spin .8s linear infinite}\n\t\t@keyframes boot-spin{to{transform:rotate(360deg)}}\n\t</style>`;
  if (!source.includes("</head>")) throw new Error("Could not locate </head> in index.html");
  source = source.replace("</head>", `${loaderStyle}\n</head>`);

  const bodyStart = '<body x-data class="flex justify-center items-center">';
  if (!source.includes(bodyStart) || !source.includes("</body>")) {
    throw new Error("Could not locate body in index.html");
  }
  source = source.replace(
    bodyStart,
    `${bodyStart}\n\t<div id="boot-loader" role="status" aria-live="polite"><div class="spinner" aria-hidden="true"></div><div>Loading...</div></div>\n\t<div id="app-shell" hidden>`
  );
  source = source.replace(
    "</body>",
    `\t</div>\n\t<script>document.addEventListener("alpine:initialized",()=>{const l=document.getElementById("boot-loader"),a=document.getElementById("app-shell");if(l)l.remove();if(a)a.hidden=false})</script>\n</body>`
  );

  return Buffer.from(source, "utf8");
}

function patchMain(buffer) {
  const source = buffer.toString("utf8");
  return Buffer.from(replaceAll(source, [
    ["./parts/button.js", "./button.js"],
    ["./parts/textInput.js", "./textInput.js"],
    ["./parts/toastNotification.js", "./toast.js"],
    ["./cubeUi/cubeUi.js", "./cubeUi.js"],
    ["./cubeUi/cubeUiSp.js", "./cubeUiSp.js"],
    ["./cubeUi/cubeFace.js", "./cubeFace.js"],
    ["./cubeUi/cubeMiniFace.js", "./cubeMiniFace.js"],
    ["./cubeUi/colorRing.js", "./colorRing.js"],
  ]), "utf8");
}

function patchCubeUiImport(buffer) {
  const source = buffer.toString("utf8");
  return Buffer.from(replaceAll(source, [
    ["../../cube-cam-scanner.js", "./cam.js"],
  ]), "utf8");
}

function patchStores(buffer) {
  let source = buffer.toString("utf8");
  const start = source.indexOf("\t\tasync request(path = \"\", queryObject = undefined) {");
  const end = source.indexOf("\t\tcreateQuery(queryObject) {", start);
  if (start < 0 || end < 0) throw new Error("Could not locate API request block in stores.js");

  const replacement = `\t\tasync request(path = "", queryObject = undefined) {\n\t\t\tconst baseUri = \`/\`\n\t\t\tconst apiPath = \`api/\${path}\`\n\t\t\tconst query = this.createQuery(queryObject)\n\t\t\tconst endpoint = \`\${baseUri}\${apiPath}\${query}\`\n\t\t\t\n\t\t\tconst response = await fetch(endpoint)\n\t\t\tconst data = await response.json()\n\t\t\t\n\t\t\tif (response.status === 200) {\n\t\t\t\tthis.toast.success("Finish!")\n\t\t\t\treturn data\n\t\t\t}\n\t\t\tif (response.status === 202) {\n\t\t\t\tthis.toast.info("Running...")\n\t\t\t\tconst status = await this.waitForExecution()\n\t\t\t\tif (status.status === "finished") this.toast.success("Finish!")\n\t\t\t\telse this.toast.error(\`Execution \${status.status}\`)\n\t\t\t\treturn data\n\t\t\t}\n\t\t\tthis.toast.error(data.error || JSON.stringify(data))\n\t\t\treturn data\n\t\t},\n\t\tasync waitForExecution() {\n\t\t\twhile (true) {\n\t\t\t\tawait new Promise(resolve => setTimeout(resolve, 250))\n\t\t\t\tconst response = await fetch("/api/status")\n\t\t\t\tconst status = await response.json()\n\t\t\t\tif (["finished", "stopped", "error"].includes(status.status)) return status\n\t\t\t}\n\t\t},\n`;
  source = source.slice(0, start) + replacement + source.slice(end);
  return Buffer.from(source, "utf8");
}

function patchTabs(buffer) {
  let source = buffer.toString("utf8");
  const start = source.indexOf("\t\tconst scrambleTab = /* html */ `");
  const end = source.indexOf("\n\t\tconst solveTab = /* html */ `", start);
  if (start < 0 || end < 0) throw new Error("Could not locate scramble tab in tabs.js");

  const replacement = `\t\tconst scrambleTab = /* html */ \`\n\t\t\t<div :id="$id(tabId + '-content')" x-show="tabContentActive($el)" x-transition:enter class="relative">\n\t\t\t\t<div class="grid gap-6 md:gap-10">\n\t\t\t\t\t<span x-data x-button='{ "text": "Random" }' @click="$store.api.scramble(0)" class="[&>*]:px-20"></span>\n\t\t\t\t</div>\n\t\t\t</div>\n\t\t\`\n`;
  source = source.slice(0, start) + replacement + source.slice(end);
  return Buffer.from(source, "utf8");
}

function patchAsset(sourcePath, buffer) {
  if (sourcePath === "index.html") return patchIndex(buffer);
  if (sourcePath === "js/alpine/main.js") return patchMain(buffer);
  if (sourcePath === "js/alpine/stores.js") return patchStores(buffer);
  if (sourcePath === "js/alpine/tabs.js") return patchTabs(buffer);
  if (sourcePath === "js/alpine/cubeUi/cubeUi.js" ||
      sourcePath === "js/alpine/cubeUi/cubeUiSp.js") {
    return patchCubeUiImport(buffer);
  }
  return buffer;
}

function bundleJavaScript(output) {
  const entry = path.join(output, "j", "main.js");
  const bundle = path.join(output, "j", "app.js");
  const npx = process.platform === "win32" ? "npx.cmd" : "npx";
  const result = spawnSync(npx, [
    "--yes",
    `esbuild@${ESBUILD_VERSION}`,
    entry,
    "--bundle",
    "--format=iife",
    "--target=es2019",
    "--minify",
    `--outfile=${bundle}`,
  ], { stdio: "inherit" });
  if (result.error) throw result.error;
  if (result.status !== 0) throw new Error(`esbuild failed with exit code ${result.status}`);

  const jsDir = path.join(output, "j");
  for (const name of fs.readdirSync(jsDir)) {
    if (!name.endsWith(".js")) continue;
    if (name === "app.js" || name === "alpine.js") continue;
    fs.rmSync(path.join(jsDir, name), { force: true });
  }
  console.log(`Bundled JS modules -> j/app.js (${fs.statSync(bundle).size} bytes)`);
}

function listFiles(root, relative = "") {
  const dir = path.join(root, relative);
  const files = [];
  for (const name of fs.readdirSync(dir)) {
    const rel = path.join(relative, name);
    const full = path.join(root, rel);
    if (fs.statSync(full).isDirectory()) files.push(...listFiles(root, rel));
    else files.push(rel.replaceAll(path.sep, "/"));
  }
  return files;
}

function gzipDeploymentAssets(output) {
  let compressedCount = 0;
  let sourceBytes = 0;
  let gzipBytes = 0;
  for (const rel of listFiles(output)) {
    if (rel.endsWith(".gz") || !GZIP_EXTENSIONS.has(path.extname(rel).toLowerCase())) continue;
    const full = path.join(output, ...rel.split("/"));
    const data = fs.readFileSync(full);
    const compressed = zlib.gzipSync(data, { level: zlib.constants.Z_BEST_COMPRESSION });
    if (compressed.length >= data.length) continue;
    fs.writeFileSync(`${full}.gz`, compressed);
    ++compressedCount;
    sourceBytes += data.length;
    gzipBytes += compressed.length;
    console.log(`gzip ${rel.padEnd(24)} ${String(data.length).padStart(7)} -> ${String(compressed.length).padStart(7)} bytes`);
  }
  return { compressedCount, sourceBytes, gzipBytes };
}

async function main() {
  const { output } = parseArgs();
  fs.rmSync(output, { recursive: true, force: true });
  fs.mkdirSync(output, { recursive: true });

  for (const asset of ASSETS) {
    const url = `https://raw.githubusercontent.com/${SOURCE_REPO}/${SOURCE_REVISION}/public/${asset.source}`;
    let data = await download(url);
    data = patchAsset(asset.source, data);

    const deploymentPath = `/${asset.target}`;
    if (Buffer.byteLength(deploymentPath, "utf8") >= 32) {
      throw new Error(`SPIFFS deployment path too long: ${deploymentPath}`);
    }

    const destination = path.join(output, ...asset.target.split("/"));
    fs.mkdirSync(path.dirname(destination), { recursive: true });
    fs.writeFileSync(destination, data);
    console.log(`${asset.source.padEnd(42)} -> ${asset.target.padEnd(18)} ${String(data.length).padStart(7)} bytes`);
  }

  bundleJavaScript(output);

  const canonicalFiles = listFiles(output);
  const canonicalBytes = canonicalFiles.reduce(
    (sum, rel) => sum + fs.statSync(path.join(output, ...rel.split("/"))).size, 0);
  const manifest = {
    sourceRepository: SOURCE_REPO,
    sourceRevision: SOURCE_REVISION,
    generatedAt: new Date().toISOString(),
    canonicalAssetCount: canonicalFiles.length + 1,
    canonicalAssetBytes: canonicalBytes,
    deploymentPaths: {
      "index.html": "index.html",
      "favicon.ico": "favicon.ico",
      "css/output.css": "c/style.css",
      "images/title-logo.png": "i/logo.png",
      "js/lib/alpinejs@3.12.3.js": "j/alpine.js",
      "bundled application JavaScript": "j/app.js",
    },
    bundle: {
      tool: `esbuild@${ESBUILD_VERSION}`,
      entry: "j/main.js",
      output: "j/app.js",
    },
    gzip: {
      enabled: true,
      extensions: Array.from(GZIP_EXTENSIONS).sort(),
      fallbackUncompressedAssetsRetained: true,
    },
    patches: [
      "flattened deployment paths for SPIFFS object-name limits",
      "inline dependency-free loading indicator until Alpine initialization",
      "bundled application ES modules into j/app.js",
      "precompressed text assets with gzip while retaining plain fallbacks",
      "async HTTP 202 execution polling via /api/status",
      "removed unsupported CubeChample-dependent scramble buttons",
    ],
  };
  fs.writeFileSync(path.join(output, "manifest.json"), JSON.stringify(manifest, null, 2) + "\n");

  const gzipStats = gzipDeploymentAssets(output);
  const finalFiles = listFiles(output);
  const finalBytes = finalFiles.reduce(
    (sum, rel) => sum + fs.statSync(path.join(output, ...rel.split("/"))).size, 0);
  console.log(`\nWeb UI source: ${SOURCE_REPO}@${SOURCE_REVISION}`);
  console.log(`Deployment files: ${finalFiles.length}`);
  console.log(`Deployment bytes: ${finalBytes}`);
  console.log(`Gzip assets: ${gzipStats.compressedCount}`);
  if (gzipStats.sourceBytes > 0) {
    console.log(`Gzip transfer bytes: ${gzipStats.sourceBytes} -> ${gzipStats.gzipBytes}`);
  }
  console.log(`Output: ${output}`);
}

main().catch(error => {
  console.error(error.stack || error.message || String(error));
  process.exit(1);
});
