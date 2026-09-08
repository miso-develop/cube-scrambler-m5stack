#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
from pathlib import Path

FLASH_SIZE = 0x400000
WEB_INSTALLER_SOURCE = Path("tools/web_installer")
# label, offset, exclusive limit, destination filename
PARTS = (
    ("bootloader", 0x000000, 0x008000, "bootloader.bin"),
    ("partitions", 0x008000, 0x009000, "partitions.bin"),
    ("boot_app0", 0x00E000, 0x010000, "boot_app0.bin"),
    ("firmware", 0x010000, 0x160000, "firmware.bin"),
    ("solver", 0x2B0000, 0x3C0000, "solver.bin"),
    ("web", 0x3C0000, 0x400000, "web.bin"),
)
DEFAULT_REPOSITORY = "https://github.com/miso-develop/cube-scrambler-m5stack"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def locate_boot_app0(explicit: Path | None) -> Path:
    if explicit is not None:
        if explicit.is_file():
            return explicit
        raise FileNotFoundError(f"boot_app0.bin not found: {explicit}")

    roots: list[Path] = []
    core_dir = os.environ.get("PLATFORMIO_CORE_DIR")
    if core_dir:
        roots.append(Path(core_dir))
    roots.extend([Path(".platformio-core"), Path.home() / ".platformio"])

    relative = Path("packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin")
    for root in roots:
        candidate = root / relative
        if candidate.is_file():
            return candidate

    raise FileNotFoundError(
        "boot_app0.bin was not found. Set PLATFORMIO_CORE_DIR or pass --boot-app0."
    )


def place(
    image: bytearray,
    offset: int,
    limit: int,
    source: Path,
    label: str,
) -> dict[str, object]:
    data = source.read_bytes()
    end = offset + len(data)
    if end > limit:
        raise ValueError(
            f"{label}: {len(data)} bytes at 0x{offset:X} ends at 0x{end:X}, "
            f"past reserved limit 0x{limit:X}"
        )
    image[offset:end] = data
    return {
        "name": label,
        "offset": offset,
        "limit": limit,
        "size": len(data),
        "free": limit - end,
        "source": source.name,
        "sha256": hashlib.sha256(data).hexdigest(),
    }


def write_web_installer(
    output: Path,
    merged: Path,
    version: str,
    ca_cert: Path,
) -> Path:
    if not WEB_INSTALLER_SOURCE.is_dir():
        raise FileNotFoundError(
            f"Web installer source directory not found: {WEB_INSTALLER_SOURCE}"
        )
    for filename in ("index.html", "installer.js"):
        source = WEB_INSTALLER_SOURCE / filename
        if not source.is_file():
            raise FileNotFoundError(f"Web installer source not found: {source}")
    if not ca_cert.is_file():
        raise FileNotFoundError(f"CA certificate not found: {ca_cert}")

    web_output = output / "web-installer"
    web_output.mkdir(parents=True, exist_ok=True)
    shutil.copy2(WEB_INSTALLER_SOURCE / "index.html", web_output / "index.html")
    shutil.copy2(WEB_INSTALLER_SOURCE / "installer.js", web_output / "installer.js")
    shutil.copy2(merged, web_output / merged.name)
    shutil.copy2(ca_cert, web_output / "ca.crt")

    # Wi-Fi provisioning is intentionally handled by installer.js after the
    # flash finishes. The firmware does not implement Improv Serial, so disable
    # ESP Web Tools' post-install Improv wait.
    manifest = {
        "name": "Cube Scrambler NanoC6",
        "version": version,
        "new_install_prompt_erase": False,
        "new_install_improv_wait_time": 0,
        "builds": [
            {
                "chipFamily": "ESP32-C6",
                "improv": False,
                "parts": [{"path": merged.name, "offset": 0}],
            }
        ],
    }
    (web_output / "manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )
    return web_output


def write_m5burner_metadata(
    output: Path, version: str, repository: str
) -> None:
    # Kept as existing deferred tooling. The current distribution target is the
    # Web Serial installer under web-installer/.
    package = output / "m5burner"
    firmware_dir = package / "firmware"
    firmware_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(
        output / "cube-scrambler-nanoc6-full.bin",
        firmware_dir / "cube_scrambler_nanoc6_0x0.bin",
    )

    metadata = {
        "name": "Cube Scrambler NanoC6",
        "description": "Standalone Cube Scrambler firmware for M5Stack NanoC6",
        "keywords": "ESP32-C6,M5Stack,NanoC6,Rubik's Cube",
        "author": "miso-develop",
        "repository": repository,
        "version": version,
        "framework": "Arduino",
    }
    (package / "m5burner.json").write_text(
        json.dumps(metadata, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Build a distributable Cube Scrambler NanoC6 full-flash bundle"
    )
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=Path(".pio/build/m5stack-nanoc6-release"),
    )
    parser.add_argument(
        "--solver", type=Path, default=Path(".pio/min2phase-tables.bin")
    )
    parser.add_argument(
        "--web",
        type=Path,
        default=Path(".pio/build/m5stack-nanoc6-release/spiffs.bin"),
    )
    parser.add_argument(
        "--ca-cert", type=Path, default=Path(".pio/web-ui/ca.crt")
    )
    parser.add_argument("--boot-app0", type=Path, default=None)
    parser.add_argument("--output", type=Path, default=Path(".pio/release"))
    parser.add_argument("--version", default="dev")
    parser.add_argument("--repository", default=DEFAULT_REPOSITORY)
    args = parser.parse_args()

    sources = {
        "bootloader": args.build_dir / "bootloader.bin",
        "partitions": args.build_dir / "partitions.bin",
        "boot_app0": locate_boot_app0(args.boot_app0),
        "firmware": args.build_dir / "firmware.bin",
        "solver": args.solver,
        "web": args.web,
    }
    for label, path in sources.items():
        if not path.is_file():
            raise FileNotFoundError(f"{label} image not found: {path}")

    output = args.output
    if output.exists():
        shutil.rmtree(output)
    output.mkdir(parents=True)

    image = bytearray(b"\xFF" * FLASH_SIZE)
    part_records: list[dict[str, object]] = []
    for label, offset, limit, destination_name in PARTS:
        source = sources[label]
        record = place(image, offset, limit, source, label)
        part_records.append(record)
        shutil.copy2(source, output / destination_name)

    merged = output / "cube-scrambler-nanoc6-full.bin"
    merged.write_bytes(image)
    if merged.stat().st_size != FLASH_SIZE:
        raise RuntimeError("merged image is not exactly 4 MiB")

    web_installer = write_web_installer(
        output=output,
        merged=merged,
        version=args.version,
        ca_cert=args.ca_cert,
    )
    write_m5burner_metadata(output, args.version, args.repository)

    release_info = {
        "name": "Cube Scrambler NanoC6",
        "version": args.version,
        "repository": args.repository,
        "flashSize": FLASH_SIZE,
        "mergedImage": {
            "path": merged.name,
            "size": merged.stat().st_size,
            "sha256": sha256(merged),
        },
        "webInstaller": {
            "path": str(web_installer),
            "publishableFiles": [
                "index.html",
                "installer.js",
                "manifest.json",
                merged.name,
                "ca.crt",
            ],
        },
        "parts": part_records,
    }
    (output / "release.json").write_text(
        json.dumps(release_info, indent=2) + "\n", encoding="utf-8"
    )

    print(f"Release bundle: {output}")
    print(f"Merged image: {merged} ({merged.stat().st_size} bytes)")
    print(f"SHA256: {sha256(merged)}")
    print(f"Publishable Web Serial installer: {web_installer}")
    for part in part_records:
        print(
            f"  {part['name']:<10} offset=0x{int(part['offset']):06X} "
            f"size={part['size']} free={part['free']}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
