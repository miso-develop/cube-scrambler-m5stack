#!/usr/bin/env python3
import argparse
import struct
import zlib
from pathlib import Path

MAGIC = b"CSN2TAB\0"
FORMAT_VERSION = 1
SOURCE_REVISION = b"0ba83a6177d816f72af1a45c9015349da597456a"
PAYLOAD_SIZE = 981_090
HEADER_FMT = "<8sHHIIIII40sI"
SECTION_FMT = "<16sIIII"
HEADER_SIZE = struct.calcsize(HEADER_FMT)
SECTION_SIZE = struct.calcsize(SECTION_FMT)


def align_up(value: int, alignment: int) -> int:
    return (value + alignment - 1) // alignment * alignment


def make_payload(size: int) -> bytes:
    payload = bytearray(size)
    state = 0x12345678
    for offset in range(0, size, 4):
        state = (state * 1664525 + 1013904223) & 0xFFFFFFFF
        chunk = state.to_bytes(4, "little")
        payload[offset:offset + min(4, size - offset)] = chunk[: min(4, size - offset)]
    return bytes(payload)


def build_image() -> bytes:
    section_count = 1
    logical_header_size = HEADER_SIZE + SECTION_SIZE * section_count
    payload_offset = align_up(logical_header_size, 64)
    payload = make_payload(PAYLOAD_SIZE)
    payload_crc = zlib.crc32(payload) & 0xFFFFFFFF
    image_size = payload_offset + len(payload)

    header = struct.pack(
        HEADER_FMT,
        MAGIC,
        FORMAT_VERSION,
        logical_header_size,
        image_size,
        payload_offset,
        len(payload),
        payload_crc,
        section_count,
        SOURCE_REVISION,
        0,
    )
    section = struct.pack(
        SECTION_FMT,
        b"probe_payload\0\0\0",
        payload_offset,
        len(payload),
        4,
        0,
    )

    padding = bytes(payload_offset - len(header) - len(section))
    return header + section + padding + payload


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "output",
        nargs="?",
        default=".pio/solver-probe.bin",
        help="Output image path (default: .pio/solver-probe.bin)",
    )
    args = parser.parse_args()

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    image = build_image()
    output.write_bytes(image)

    payload_offset = align_up(HEADER_SIZE + SECTION_SIZE, 64)
    payload = image[payload_offset:]
    print(f"Wrote: {output}")
    print(f"Image size: {len(image)} bytes")
    print(f"Payload size: {len(payload)} bytes")
    print(f"Payload CRC32: 0x{zlib.crc32(payload) & 0xFFFFFFFF:08X}")
    print(f"Source revision: {SOURCE_REVISION.decode()}")


if __name__ == "__main__":
    main()
