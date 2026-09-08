#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from pathlib import Path

FLASH_SIZE = 0x400000
APP_ALIGNMENT = 0x10000
DATA_ALIGNMENT = 0x1000
ARDUINO_BOOT_APP0_OFFSET = 0xE000
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


def find_partition(partitions: list[dict[str, object]], name: str) -> dict[str, object]:
    for partition in partitions:
        if partition["name"] == name:
            return partition
    raise ValueError(f"partition not found: {name}")


def check_layout(partitions: list[dict[str, object]]) -> None:
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
        if end > FLASH_SIZE:
            raise ValueError(f"{name}: ends beyond 4MB flash at 0x{end:X}")
        previous_end = end

    if previous_end != FLASH_SIZE:
        raise ValueError(
            f"final partition ends at 0x{previous_end:X}, expected 0x{FLASH_SIZE:X}"
        )

    otadata = find_partition(partitions, "otadata")
    if int(otadata["offset"]) != ARDUINO_BOOT_APP0_OFFSET:
        raise ValueError(
            "otadata must start at 0xE000 because Arduino/pioarduino flashes "
            "boot_app0.bin at that fixed address"
        )
    if int(otadata["size"]) != OTA_DATA_SIZE:
        raise ValueError("otadata must be exactly 0x2000 bytes")


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


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate final NanoC6 flash layout and images")
    parser.add_argument(
        "--partitions",
        default="partitions/cube_scrambler_4mb.csv",
        type=Path,
    )
    parser.add_argument(
        "--firmware",
        default=".pio/build/m5stack-nanoc6/firmware.bin",
        type=Path,
    )
    parser.add_argument(
        "--solver",
        default=".pio/min2phase-tables.bin",
        type=Path,
    )
    parser.add_argument(
        "--web",
        default=".pio/build/m5stack-nanoc6/spiffs.bin",
        type=Path,
    )
    args = parser.parse_args()

    try:
        partitions = load_partitions(args.partitions)
        check_layout(partitions)
        ota0 = find_partition(partitions, "ota_0")
        ota1 = find_partition(partitions, "ota_1")
        if ota0["size"] != ota1["size"]:
            raise ValueError("ota_0 and ota_1 must have equal capacity")
        check_image(args.firmware, ota0)
        check_image(args.solver, find_partition(partitions, "solver"))
        check_image(args.web, find_partition(partitions, "web"))
    except (OSError, ValueError) as exc:
        print(f"FLASH LAYOUT CHECK: FAIL: {exc}")
        return 1

    print("FLASH LAYOUT CHECK: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
