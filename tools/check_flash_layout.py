#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from pathlib import Path

from device_profiles import DeviceProfile, DeviceProfileError, load_device_profile

APP_ALIGNMENT = 0x10000
DATA_ALIGNMENT = 0x1000
OTA_DATA_SIZE = 0x2000


def parse_int(value: str) -> int:
    value = value.strip()
    if not value:
        raise ValueError("empty integer")
    suffix = value[-1].lower()
    if suffix == "k":
        return int(value[:-1], 0) * 1024
    if suffix == "m":
        return int(value[:-1], 0) * 1024 * 1024
    return int(value, 0)


def load_partitions(path: Path) -> list[dict[str, object]]:
    partitions: list[dict[str, object]] = []
    with path.open("r", encoding="utf-8", newline="") as handle:
        for raw in csv.reader(handle):
            if not raw or raw[0].lstrip().startswith("#"):
                continue
            row = [item.strip() for item in raw]
            if len(row) < 5:
                raise ValueError(f"invalid partition row: {raw}")
            name, ptype, subtype, offset_text, size_text = row[:5]
            partitions.append(
                {
                    "name": name,
                    "type": ptype,
                    "subtype": subtype,
                    "offset": parse_int(offset_text),
                    "size": parse_int(size_text),
                }
            )
    return partitions


def find_partition(
    partitions: list[dict[str, object]], name: str
) -> dict[str, object]:
    for partition in partitions:
        if partition["name"] == name:
            return partition
    raise ValueError(f"partition not found: {name}")


def check_layout(
    partitions: list[dict[str, object]],
    flash_size: int,
) -> None:
    previous_end = 0
    for partition in sorted(partitions, key=lambda item: int(item["offset"])):
        name = str(partition["name"])
        ptype = str(partition["type"])
        offset = int(partition["offset"])
        size = int(partition["size"])
        alignment = APP_ALIGNMENT if ptype == "app" else DATA_ALIGNMENT
        if offset % alignment != 0:
            raise ValueError(
                f"{name}: offset 0x{offset:X} is not aligned to 0x{alignment:X}"
            )
        if offset < previous_end:
            raise ValueError(f"{name}: overlaps previous partition")
        end = offset + size
        if end > flash_size:
            raise ValueError(
                f"{name}: ends beyond declared flash size 0x{flash_size:X} at 0x{end:X}"
            )
        previous_end = end

    if previous_end != flash_size:
        raise ValueError(
            f"final partition ends at 0x{previous_end:X}, expected 0x{flash_size:X}"
        )


def _check_partition_matches_part(
    partitions: list[dict[str, object]],
    partition_name: str,
    profile: DeviceProfile,
    part_name: str,
) -> None:
    partition = find_partition(partitions, partition_name)
    part = profile.part(part_name)
    offset = int(partition["offset"])
    end = offset + int(partition["size"])
    if offset != part.offset or end != part.limit:
        raise ValueError(
            f"{partition_name}: partition range 0x{offset:X}-0x{end:X} "
            f"does not match profile part {part_name} "
            f"0x{part.offset:X}-0x{part.limit:X}"
        )


def check_profile_partition_contract(
    partitions: list[dict[str, object]],
    profile: DeviceProfile,
) -> None:
    required_parts = {"firmware", "boot_app0", "solver", "web"}
    missing = sorted(required_parts - {part.name for part in profile.parts})
    if missing:
        raise ValueError(
            "profile missing flash parts required by layout validation: "
            + ", ".join(missing)
        )

    check_layout(partitions, profile.flash_size)

    ota0 = find_partition(partitions, "ota_0")
    ota1 = find_partition(partitions, "ota_1")
    if ota0["size"] != ota1["size"]:
        raise ValueError("ota_0 and ota_1 must have equal capacity")

    _check_partition_matches_part(partitions, "ota_0", profile, "firmware")
    _check_partition_matches_part(partitions, "solver", profile, "solver")
    _check_partition_matches_part(partitions, "web", profile, "web")

    otadata = find_partition(partitions, "otadata")
    boot_app0 = profile.part("boot_app0")
    if int(otadata["offset"]) != boot_app0.offset:
        raise ValueError(
            f"otadata: offset 0x{int(otadata['offset']):X} does not match "
            f"profile boot_app0 offset 0x{boot_app0.offset:X}"
        )
    if int(otadata["size"]) != OTA_DATA_SIZE:
        raise ValueError("otadata must be exactly 0x2000 bytes")
    if int(otadata["offset"]) + int(otadata["size"]) != boot_app0.limit:
        raise ValueError(
            "otadata end must match the profile boot_app0 reserved limit"
        )


def check_image(image: Path, partition: dict[str, object]) -> None:
    if not image.is_file():
        raise ValueError(f"image not found: {image}")
    size = image.stat().st_size
    capacity = int(partition["size"])
    free = capacity - size
    if free < 0:
        raise ValueError(
            f"{image}: {size} bytes exceeds {partition['name']} capacity {capacity} bytes"
        )
    print(
        f"{partition['name']}: image={size} capacity={capacity} free={free} "
        f"({size / capacity * 100:.1f}% used)"
    )


def _profile_args(parser: argparse.ArgumentParser) -> None:
    selection = parser.add_mutually_exclusive_group(required=True)
    selection.add_argument("--device")
    selection.add_argument("--profile", type=Path)


def _load_selected_profile(args: argparse.Namespace) -> DeviceProfile:
    if args.device is not None:
        return load_device_profile(device_id=args.device)
    return load_device_profile(profile_path=args.profile)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate a Cube Scrambler device flash layout and images"
    )
    _profile_args(parser)
    parser.add_argument("--firmware", type=Path, default=None)
    parser.add_argument(
        "--solver",
        type=Path,
        default=Path(".pio/min2phase-tables.bin"),
    )
    parser.add_argument("--web", type=Path, default=None)
    args = parser.parse_args()

    try:
        profile = _load_selected_profile(args)
        partitions_path = Path(profile.partition_file)
        build_dir = Path(".pio") / "build" / profile.platformio_release_env
        firmware = args.firmware or build_dir / "firmware.bin"
        web = args.web or build_dir / "spiffs.bin"

        partitions = load_partitions(partitions_path)
        check_profile_partition_contract(partitions, profile)
        check_image(firmware, find_partition(partitions, "ota_0"))
        check_image(args.solver, find_partition(partitions, "solver"))
        check_image(web, find_partition(partitions, "web"))
    except (DeviceProfileError, OSError, ValueError) as exc:
        print(f"FLASH LAYOUT CHECK: FAIL: {exc}")
        return 1

    print("FLASH LAYOUT CHECK: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
