#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import html
import json
import os
import shutil
from pathlib import Path

from check_flash_layout import check_profile_partition_contract, load_partitions
from device_profiles import DeviceProfile, DeviceProfileError, load_device_profile

WEB_INSTALLER_SOURCE = Path("tools/web_installer")
DEFAULT_REPOSITORY = "https://github.com/miso-develop/cube-scrambler-m5stack"
_REQUIRED_BUNDLE_PARTS = {
    "bootloader",
    "partitions",
    "boot_app0",
    "firmware",
    "solver",
    "web",
}


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

    relative = Path(
        "packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"
    )
    for root in roots:
        candidate = root / relative
        if candidate.is_file():
            return candidate

    raise FileNotFoundError(
        "boot_app0.bin was not found. Set PLATFORMIO_CORE_DIR or pass --boot-app0."
    )


def validate_bundle_profile(profile: DeviceProfile) -> None:
    names = {part.name for part in profile.parts}
    missing = sorted(_REQUIRED_BUNDLE_PARTS - names)
    unsupported = sorted(names - _REQUIRED_BUNDLE_PARTS)
    if missing:
        raise ValueError(
            "profile missing flash parts required by release bundling: "
            + ", ".join(missing)
        )
    if unsupported:
        raise ValueError(
            "profile contains flash parts unsupported by release bundling: "
            + ", ".join(unsupported)
        )


def validate_sources(
    profile: DeviceProfile,
    sources: dict[str, Path],
) -> None:
    for part in profile.parts:
        source = sources[part.name]
        if not source.is_file():
            raise FileNotFoundError(f"{part.name} image not found: {source}")
        size = source.stat().st_size
        capacity = part.limit - part.offset
        if size > capacity:
            raise ValueError(
                f"{part.name}: {size} bytes exceeds reserved capacity "
                f"{capacity} bytes for 0x{part.offset:X}-0x{part.limit:X}"
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


def _render_required_template(source: Path, replacements: dict[str, str]) -> str:
    text = source.read_text(encoding="utf-8")
    for token, replacement in replacements.items():
        if token not in text:
            raise ValueError(
                f"installer template {source} missing required token: {token}"
            )
        text = text.replace(token, replacement)
    if "__CUBE_" in text:
        raise ValueError(
            f"installer template {source} contains unresolved Cube placeholders"
        )
    return text


def write_web_installer(
    output: Path,
    merged: Path,
    version: str,
    ca_cert: Path,
    profile: DeviceProfile,
) -> Path:
    if not WEB_INSTALLER_SOURCE.is_dir():
        raise FileNotFoundError(
            f"Web installer source directory not found: {WEB_INSTALLER_SOURCE}"
        )
    index_source = WEB_INSTALLER_SOURCE / "index.html"
    script_source = WEB_INSTALLER_SOURCE / "installer.js"
    for source in (index_source, script_source):
        if not source.is_file():
            raise FileNotFoundError(f"Web installer source not found: {source}")
    if not ca_cert.is_file():
        raise FileNotFoundError(f"CA certificate not found: {ca_cert}")

    recovery = profile.installer_recovery_guidance
    rendered_index = _render_required_template(
        index_source,
        {
            "__CUBE_DEVICE_DISPLAY_NAME__": html.escape(profile.display_name),
            "__CUBE_DEVICE_SERIAL_LABEL_HTML__": html.escape(
                profile.installer_serial_label
            ),
            "__CUBE_FULL_IMAGE__": html.escape(merged.name),
            "__CUBE_RECOVERY_CLASS__": "" if recovery else "hidden",
            "__CUBE_RECOVERY_GUIDANCE__": html.escape(recovery or ""),
        },
    )
    rendered_script = _render_required_template(
        script_source,
        {
            "__CUBE_DEVICE_SERIAL_LABEL_JS__": json.dumps(
                profile.installer_serial_label, ensure_ascii=False
            )[1:-1],
        },
    )

    web_output = output / "web-installer"
    web_output.mkdir(parents=True, exist_ok=True)
    (web_output / "index.html").write_text(rendered_index, encoding="utf-8")
    (web_output / "installer.js").write_text(rendered_script, encoding="utf-8")
    shutil.copy2(merged, web_output / merged.name)
    shutil.copy2(ca_cert, web_output / "ca.crt")

    manifest = {
        "name": profile.release_name,
        "version": version,
        "new_install_prompt_erase": False,
        "new_install_improv_wait_time": 0,
        "builds": [
            {
                "chipFamily": profile.chip_family,
                "improv": False,
                "parts": [{"path": merged.name, "offset": 0}],
            }
        ],
    }
    (web_output / "manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )
    return web_output


def _legacy_m5burner_filename(merged: Path) -> str:
    stem = merged.stem
    if stem.endswith("-full"):
        stem = stem[: -len("-full")]
    return stem.replace("-", "_") + "_0x0.bin"


def write_m5burner_metadata(
    output: Path,
    merged: Path,
    profile: DeviceProfile,
    version: str,
    repository: str,
) -> None:
    # Kept as existing deferred tooling. The current distribution target is the
    # Web Serial installer under web-installer/.
    package = output / "m5burner"
    firmware_dir = package / "firmware"
    firmware_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(merged, firmware_dir / _legacy_m5burner_filename(merged))

    release_target = profile.release_name
    if release_target.startswith("Cube Scrambler "):
        release_target = release_target[len("Cube Scrambler ") :]
    metadata = {
        "name": profile.release_name,
        "description": f"Standalone Cube Scrambler firmware for {release_target}",
        "keywords": f"{profile.chip_family},M5Stack,{release_target},Rubik's Cube",
        "author": "miso-develop",
        "repository": repository,
        "version": version,
        "framework": "Arduino",
    }
    (package / "m5burner.json").write_text(
        json.dumps(metadata, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )


def _profile_args(parser: argparse.ArgumentParser) -> None:
    selection = parser.add_mutually_exclusive_group(required=True)
    selection.add_argument("--device")
    selection.add_argument("--profile", type=Path)


def _load_selected_profile(args: argparse.Namespace) -> DeviceProfile:
    if args.device is not None:
        return load_device_profile(device_id=args.device)
    return load_device_profile(profile_path=args.profile)


def build_bundle(
    *,
    profile: DeviceProfile,
    build_dir: Path,
    solver: Path,
    web: Path,
    ca_cert: Path,
    boot_app0: Path | None,
    output: Path,
    version: str,
    repository: str,
) -> Path:
    validate_bundle_profile(profile)

    partitions = load_partitions(Path(profile.partition_file))
    check_profile_partition_contract(partitions, profile)

    sources = {
        "bootloader": build_dir / "bootloader.bin",
        "partitions": build_dir / "partitions.bin",
        "boot_app0": locate_boot_app0(boot_app0),
        "firmware": build_dir / "firmware.bin",
        "solver": solver,
        "web": web,
    }
    validate_sources(profile, sources)
    if not ca_cert.is_file():
        raise FileNotFoundError(f"CA certificate not found: {ca_cert}")

    if output.exists():
        shutil.rmtree(output)
    output.mkdir(parents=True)

    image = bytearray(b"\xFF" * profile.flash_size)
    part_records: list[dict[str, object]] = []
    for part in profile.parts:
        source = sources[part.name]
        record = place(image, part.offset, part.limit, source, part.name)
        part_records.append(record)
        shutil.copy2(source, output / part.destination)

    merged = output / profile.full_image
    merged.write_bytes(image)
    if merged.stat().st_size != profile.flash_size:
        raise RuntimeError(
            f"merged image is not exactly declared flash size {profile.flash_size}"
        )

    web_installer = write_web_installer(
        output=output,
        merged=merged,
        version=version,
        ca_cert=ca_cert,
        profile=profile,
    )
    write_m5burner_metadata(output, merged, profile, version, repository)

    release_info = {
        "name": profile.release_name,
        "deviceId": profile.id,
        "version": version,
        "repository": repository,
        "flashSize": profile.flash_size,
        "platformioReleaseEnv": profile.platformio_release_env,
        "partitionFile": profile.partition_file,
        "mergedImage": {
            "path": merged.name,
            "size": merged.stat().st_size,
            "sha256": sha256(merged),
        },
        "webInstaller": {
            "path": str(web_installer),
            "deviceDisplayName": profile.display_name,
            "serialLabel": profile.installer_serial_label,
            "manifest": "manifest.json",
            "fullImage": merged.name,
            "recoveryGuidance": profile.installer_recovery_guidance,
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
    print(f"Device: {profile.id} ({profile.display_name})")
    print(f"Merged image: {merged} ({merged.stat().st_size} bytes)")
    print(f"SHA256: {sha256(merged)}")
    print(f"Publishable Web Serial installer: {web_installer}")
    for part in part_records:
        print(
            f"  {part['name']:<10} offset=0x{int(part['offset']):06X} "
            f"size={part['size']} free={part['free']}"
        )
    return merged


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Build a distributable Cube Scrambler device full-flash bundle"
    )
    _profile_args(parser)
    parser.add_argument("--build-dir", type=Path, default=None)
    parser.add_argument(
        "--solver", type=Path, default=Path(".pio/min2phase-tables.bin")
    )
    parser.add_argument("--web", type=Path, default=None)
    parser.add_argument(
        "--ca-cert", type=Path, default=Path(".pio/web-ui/ca.crt")
    )
    parser.add_argument("--boot-app0", type=Path, default=None)
    parser.add_argument("--output", type=Path, default=Path(".pio/release"))
    parser.add_argument("--version", default="dev")
    parser.add_argument("--repository", default=DEFAULT_REPOSITORY)
    args = parser.parse_args()

    try:
        profile = _load_selected_profile(args)
        build_dir = args.build_dir or (
            Path(".pio") / "build" / profile.platformio_release_env
        )
        web = args.web or build_dir / "spiffs.bin"
        build_bundle(
            profile=profile,
            build_dir=build_dir,
            solver=args.solver,
            web=web,
            ca_cert=args.ca_cert,
            boot_app0=args.boot_app0,
            output=args.output,
            version=args.version,
            repository=args.repository,
        )
    except (DeviceProfileError, OSError, RuntimeError, ValueError) as exc:
        print(f"RELEASE BUNDLE: FAIL: {exc}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
