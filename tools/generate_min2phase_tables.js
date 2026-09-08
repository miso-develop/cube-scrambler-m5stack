#!/usr/bin/env node
"use strict";

// Generates the Flash-resident Min2Phase lookup-table image used by NanoC6.
// Reference source:
//   cs0x7f/min2phase.js@0ba83a6177d816f72af1a45c9015349da597456a
// Selected upstream license: MIT (see docs/THIRD_PARTY.md).
//
// The pinned upstream source is downloaded (or supplied with --source), patched
// in memory only to expose its already-generated internal tables, then initFull()
// is executed on the host. The resulting arrays are compacted into fixed-width
// little-endian sections. Upstream source itself is not embedded in firmware.

const crypto = require("crypto");
const fs = require("fs");
const https = require("https");
const path = require("path");
const vm = require("vm");

const REVISION = "0ba83a6177d816f72af1a45c9015349da597456a";
const EXPECTED_GIT_BLOB_SHA1 = "b8a641b03cea8e6d7a6b0f2a6da74ff98e03475e";
const SOURCE_URL = `https://raw.githubusercontent.com/cs0x7f/min2phase.js/${REVISION}/min2phase.js`;
const DEFAULT_OUTPUT = path.join(".pio", "min2phase-tables.bin");
const DEFAULT_CACHE = path.join(".pio", `min2phase-${REVISION}.js`);

const MAGIC = Buffer.from("CSN2TAB\0", "ascii");
const FORMAT_VERSION = 1;
const IMAGE_HEADER_SIZE = 76;
const SECTION_ENTRY_SIZE = 32;
const MAX_SECTIONS = 32;
const PAYLOAD_ALIGNMENT = 64;
const SOLVER_PARTITION_SIZE = 0x110000;

// count is the flattened element count after upstream initFull().
// elementSize is the compact representation consumed by the C++ port.
const TABLES = [
  { name: "TwstMove", count: 324 * 18, elementSize: 2 },
  { name: "FlipMove", count: 336 * 18, elementSize: 2 },
  { name: "SliceMove", count: 495 * 18, elementSize: 2 },
  { name: "SliceConj", count: 495 * 8, elementSize: 2 },
  { name: "CPermMove", count: 2768 * 10, elementSize: 2 },
  { name: "EPermMove", count: 2768 * 10, elementSize: 2 },
  { name: "MPermMove", count: 24 * 10, elementSize: 1 },
  { name: "MPermConj", count: 24 * 16, elementSize: 1 },
  { name: "CCombPMove", count: 140 * 10, elementSize: 1 },
  { name: "CCombPConj", count: 140 * 16, elementSize: 1 },
  { name: "SymMult", count: 16 * 16, elementSize: 1 },
  { name: "SymMultInv", count: 16 * 16, elementSize: 1 },
  { name: "SymMove", count: 16 * 18, elementSize: 1 },
  { name: "SymMoveUD", count: 16 * 18, elementSize: 1 },
  { name: "Sym8Move", count: 18 * 8, elementSize: 1 },
  { name: "FlipS2R", count: 336, elementSize: 2 },
  { name: "FlipR2S", count: 2048, elementSize: 2 },
  { name: "FlipSelfSym", count: 336, elementSize: 1 },
  { name: "FlipS2RF", count: 336 * 8, elementSize: 2 },
  { name: "TwstS2R", count: 324, elementSize: 2 },
  { name: "TwstR2S", count: 2187, elementSize: 2 },
  { name: "TwstSelfSym", count: 324, elementSize: 1 },
  { name: "EPermS2R", count: 2768, elementSize: 2 },
  { name: "EPermR2S", count: 40320, elementSize: 2 },
  { name: "PermSelfSym", count: 2768, elementSize: 2 },
  { name: "Perm2CombP", count: 2768, elementSize: 1 },
  { name: "PermInvEdgeSym", count: 2768, elementSize: 2 },
  { name: "TwstFlipPrun", count: ((2048 * 324) >> 3) + 1, elementSize: 4 },
  { name: "SliceTwstPrun", count: ((495 * 324) >> 3) + 1, elementSize: 4 },
  { name: "SliceFlipPrun", count: ((495 * 336) >> 3) + 1, elementSize: 4 },
  { name: "MCPermPrun", count: ((24 * 2768) >> 3) + 1, elementSize: 4 },
  { name: "EPermCCombPPrun", count: ((140 * 2768) >> 3) + 1, elementSize: 4 },
];

function alignUp(value, alignment) {
  return Math.ceil(value / alignment) * alignment;
}

function parseArgs(argv) {
  const args = { output: DEFAULT_OUTPUT, source: null, noDownload: false };
  for (let i = 2; i < argv.length; ++i) {
    const arg = argv[i];
    if (arg === "--output") {
      args.output = argv[++i];
    } else if (arg === "--source") {
      args.source = argv[++i];
    } else if (arg === "--no-download") {
      args.noDownload = true;
    } else if (arg === "--help" || arg === "-h") {
      console.log("Usage: node tools/generate_min2phase_tables.js [--output FILE] [--source min2phase.js] [--no-download]");
      process.exit(0);
    } else {
      throw new Error(`Unknown argument: ${arg}`);
    }
  }
  return args;
}

function downloadText(url) {
  return new Promise((resolve, reject) => {
    https.get(url, { headers: { "User-Agent": "cube-scrambler-nanoc6-table-generator" } }, (response) => {
      if (response.statusCode >= 300 && response.statusCode < 400 && response.headers.location) {
        response.resume();
        downloadText(response.headers.location).then(resolve, reject);
        return;
      }
      if (response.statusCode !== 200) {
        reject(new Error(`Download failed: HTTP ${response.statusCode}`));
        response.resume();
        return;
      }
      response.setEncoding("utf8");
      let text = "";
      response.on("data", (chunk) => { text += chunk; });
      response.on("end", () => resolve(text));
    }).on("error", reject);
  });
}

function gitBlobSha1(text) {
  const bytes = Buffer.from(text, "utf8");
  const header = Buffer.from(`blob ${bytes.length}\0`, "ascii");
  return crypto.createHash("sha1").update(header).update(bytes).digest("hex");
}

async function loadPinnedSource(args) {
  let sourcePath = args.source;
  if (!sourcePath) {
    sourcePath = DEFAULT_CACHE;
    if (!fs.existsSync(sourcePath)) {
      if (args.noDownload) {
        throw new Error(`Pinned source cache not found: ${sourcePath}`);
      }
      console.log(`Downloading pinned min2phase.js: ${REVISION}`);
      const source = await downloadText(SOURCE_URL);
      fs.mkdirSync(path.dirname(sourcePath), { recursive: true });
      fs.writeFileSync(sourcePath, source, "utf8");
    }
  }

  const source = fs.readFileSync(sourcePath, "utf8");
  const blobSha = gitBlobSha1(source);
  if (blobSha !== EXPECTED_GIT_BLOB_SHA1) {
    throw new Error(
      `Pinned source Git blob mismatch: expected ${EXPECTED_GIT_BLOB_SHA1}, got ${blobSha}`
    );
  }
  console.log(`Verified upstream Git blob: ${blobSha}`);
  return source;
}

function patchSourceForTableExport(source) {
  const marker = "\treturn {\n\t\tSearch: Search,";
  if (!source.includes(marker)) {
    throw new Error("Pinned min2phase.js export marker was not found");
  }

  const exportedNames = TABLES.map((table) => table.name).join(", ");
  const replacement =
    "\treturn {\n" +
    "\t\t_exportTables: function() {\n" +
    `\t\t\treturn { ${exportedNames} };\n` +
    "\t\t},\n" +
    "\t\tSearch: Search,";
  return source.replace(marker, replacement);
}

function evaluatePatchedSource(source) {
  const context = {
    module: { exports: {} },
    exports: {},
    console,
    Math,
  };
  vm.createContext(context);
  vm.runInContext(source, context, {
    filename: `min2phase-${REVISION}.js`,
    timeout: 120000,
  });
  const min2phase = context.module.exports;
  if (!min2phase || typeof min2phase.initFull !== "function" ||
      typeof min2phase._exportTables !== "function") {
    throw new Error("Patched min2phase.js did not expose the expected API");
  }
  return min2phase;
}

function flattenArray(value, output) {
  if (Array.isArray(value)) {
    for (const item of value) flattenArray(item, output);
    return;
  }
  if (value === undefined || value === null) {
    output.push(0);
    return;
  }
  if (!Number.isInteger(value)) {
    throw new Error(`Non-integer table value: ${value}`);
  }
  output.push(value);
}

function encodeTable(definition, value) {
  const flattened = [];
  flattenArray(value, flattened);
  if (flattened.length !== definition.count) {
    throw new Error(
      `${definition.name}: expected ${definition.count} elements, got ${flattened.length}`
    );
  }

  const buffer = Buffer.alloc(definition.count * definition.elementSize);
  for (let i = 0; i < flattened.length; ++i) {
    const value = flattened[i];
    const offset = i * definition.elementSize;
    if (definition.elementSize === 1) {
      if (value < 0 || value > 0xff) {
        throw new Error(`${definition.name}[${i}] does not fit uint8: ${value}`);
      }
      buffer.writeUInt8(value, offset);
    } else if (definition.elementSize === 2) {
      if (value < 0 || value > 0xffff) {
        throw new Error(`${definition.name}[${i}] does not fit uint16: ${value}`);
      }
      buffer.writeUInt16LE(value, offset);
    } else if (definition.elementSize === 4) {
      buffer.writeUInt32LE(value >>> 0, offset);
    } else {
      throw new Error(`Unsupported element size: ${definition.elementSize}`);
    }
  }
  return buffer;
}

let crcTable = null;
function crc32(buffer) {
  if (!crcTable) {
    crcTable = new Uint32Array(256);
    for (let i = 0; i < 256; ++i) {
      let value = i;
      for (let bit = 0; bit < 8; ++bit) {
        value = (value >>> 1) ^ ((value & 1) ? 0xedb88320 : 0);
      }
      crcTable[i] = value >>> 0;
    }
  }

  let crc = 0xffffffff;
  for (const byte of buffer) {
    crc = (crc >>> 8) ^ crcTable[(crc ^ byte) & 0xff];
  }
  return (crc ^ 0xffffffff) >>> 0;
}

function buildImage(exportedTables) {
  if (TABLES.length > MAX_SECTIONS) {
    throw new Error(`Section count ${TABLES.length} exceeds format limit ${MAX_SECTIONS}`);
  }

  const logicalHeaderSize = IMAGE_HEADER_SIZE + TABLES.length * SECTION_ENTRY_SIZE;
  const payloadOffset = alignUp(logicalHeaderSize, PAYLOAD_ALIGNMENT);
  const sections = [];
  let cursor = payloadOffset;

  for (const definition of TABLES) {
    const alignment = Math.max(4, definition.elementSize);
    cursor = alignUp(cursor, alignment);
    const encoded = encodeTable(definition, exportedTables[definition.name]);
    sections.push({ ...definition, offset: cursor, data: encoded });
    cursor += encoded.length;
  }

  const imageSize = cursor;
  if (imageSize > SOLVER_PARTITION_SIZE) {
    throw new Error(
      `Solver image ${imageSize} bytes exceeds partition ${SOLVER_PARTITION_SIZE} bytes`
    );
  }

  const image = Buffer.alloc(imageSize, 0);
  for (const section of sections) {
    section.data.copy(image, section.offset);
  }

  const payloadSize = imageSize - payloadOffset;
  const payloadCrc = crc32(image.subarray(payloadOffset, imageSize));

  MAGIC.copy(image, 0);
  image.writeUInt16LE(FORMAT_VERSION, 8);
  image.writeUInt16LE(logicalHeaderSize, 10);
  image.writeUInt32LE(imageSize, 12);
  image.writeUInt32LE(payloadOffset, 16);
  image.writeUInt32LE(payloadSize, 20);
  image.writeUInt32LE(payloadCrc, 24);
  image.writeUInt32LE(TABLES.length, 28);
  Buffer.from(REVISION, "ascii").copy(image, 32, 0, 40);
  image.writeUInt32LE(0, 72);

  for (let i = 0; i < sections.length; ++i) {
    const section = sections[i];
    const entryOffset = IMAGE_HEADER_SIZE + i * SECTION_ENTRY_SIZE;
    const nameBuffer = Buffer.from(section.name, "ascii");
    if (nameBuffer.length >= 16) {
      throw new Error(`Section name must be at most 15 bytes: ${section.name}`);
    }
    nameBuffer.copy(image, entryOffset);
    image.writeUInt32LE(section.offset, entryOffset + 16);
    image.writeUInt32LE(section.data.length, entryOffset + 20);
    image.writeUInt32LE(section.elementSize, entryOffset + 24);
    image.writeUInt32LE(0, entryOffset + 28);
  }

  return { image, sections, payloadOffset, payloadSize, payloadCrc };
}

async function main() {
  const args = parseArgs(process.argv);
  const source = await loadPinnedSource(args);
  const patched = patchSourceForTableExport(source);
  const min2phase = evaluatePatchedSource(patched);

  console.log("Generating full Min2Phase tables on host...");
  const started = Date.now();
  min2phase.initFull();
  const exportedTables = min2phase._exportTables();
  const initMs = Date.now() - started;

  const result = buildImage(exportedTables);
  fs.mkdirSync(path.dirname(args.output), { recursive: true });
  fs.writeFileSync(args.output, result.image);

  console.log(`Wrote: ${args.output}`);
  console.log(`Source revision: ${REVISION}`);
  console.log(`Table generation: ${initMs} ms`);
  console.log(`Sections: ${result.sections.length}`);
  console.log(`Image size: ${result.image.length} bytes`);
  console.log(`Payload size: ${result.payloadSize} bytes`);
  console.log(`Payload CRC32: 0x${result.payloadCrc.toString(16).toUpperCase().padStart(8, "0")}`);
  console.log(`Partition free after image: ${SOLVER_PARTITION_SIZE - result.image.length} bytes`);
  for (const section of result.sections) {
    console.log(
      `  ${section.name.padEnd(15)} ${String(section.data.length).padStart(7)} bytes ` +
      `(${section.count} x ${section.elementSize})`
    );
  }
}

main().catch((error) => {
  console.error(`ERROR: ${error.stack || error.message}`);
  process.exit(1);
});
