#!/usr/bin/env python3
from __future__ import annotations

import json
import re
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
from typing import Any

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_PROFILE_DIR = REPOSITORY_ROOT / "config" / "devices"

_DEVICE_ID_RE = re.compile(r"^[a-z0-9](?:[a-z0-9-]*[a-z0-9])?$")
_SUPPORTED_CHIP_FAMILIES = {
    "ESP32",
    "ESP32-S2",
    "ESP32-S3",
    "ESP32-C3",
    "ESP32-C6",
    "ESP32-H2",
}


class DeviceProfileError(ValueError):
    """Raised when a device profile is missing, ambiguous, or unsafe."""


@dataclass(frozen=True)
class FlashPart:
    name: str
    offset: int
    limit: int
    destination: str


@dataclass(frozen=True)
class DeviceProfile:
    schema_version: int
    id: str
    display_name: str
    chip_family: str
    flash_size: int
    platformio_release_env: str
    partition_file: str
    full_image: str
    parts: tuple[FlashPart, ...]

    def part(self, name: str) -> FlashPart:
        for part in self.parts:
            if part.name == name:
                return part
        raise DeviceProfileError(f"flash part not found: {name}")


def _reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise DeviceProfileError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def _require_object(value: Any, field: str) -> dict[str, Any]:
    if not isinstance(value, dict):
        raise DeviceProfileError(f"{field} must be an object")
    return value


def _require_exact_keys(
    value: dict[str, Any], field: str, required: set[str]
) -> None:
    missing = sorted(required - value.keys())
    unknown = sorted(value.keys() - required)
    if missing:
        raise DeviceProfileError(f"{field} missing required fields: {', '.join(missing)}")
    if unknown:
        raise DeviceProfileError(f"{field} contains unsupported fields: {', '.join(unknown)}")


def _require_string(value: Any, field: str) -> str:
    if not isinstance(value, str) or not value.strip():
        raise DeviceProfileError(f"{field} must be a non-empty string")
    if value != value.strip():
        raise DeviceProfileError(f"{field} must not have surrounding whitespace")
    return value


def _require_int(value: Any, field: str, *, minimum: int = 0) -> int:
    if type(value) is not int or value < minimum:
        raise DeviceProfileError(f"{field} must be an integer >= {minimum}")
    return value


def _require_device_id(value: Any, field: str = "id") -> str:
    device_id = _require_string(value, field)
    if not _DEVICE_ID_RE.fullmatch(device_id):
        raise DeviceProfileError(
            f"{field} must use lowercase letters, digits, and path-safe hyphens"
        )
    return device_id


def _require_repo_relative_path(value: Any, field: str) -> str:
    text = _require_string(value, field)
    if "\\" in text:
        raise DeviceProfileError(f"{field} must use forward slashes")
    path = PurePosixPath(text)
    if path.is_absolute() or path.as_posix() != text or ".." in path.parts:
        raise DeviceProfileError(f"{field} must be a normalized repository-relative path")
    return text


def _require_filename(value: Any, field: str, *, suffix: str | None = None) -> str:
    text = _require_string(value, field)
    if "\\" in text:
        raise DeviceProfileError(f"{field} must be a filename, not a path")
    path = PurePosixPath(text)
    if path.as_posix() != text or path.name != text or text in {".", ".."}:
        raise DeviceProfileError(f"{field} must be a filename, not a path")
    if suffix is not None and not text.endswith(suffix):
        raise DeviceProfileError(f"{field} must end with {suffix}")
    return text


def parse_device_profile(data: Any, *, expected_id: str | None = None) -> DeviceProfile:
    root = _require_object(data, "profile")
    _require_exact_keys(
        root,
        "profile",
        {
            "schemaVersion",
            "id",
            "displayName",
            "chipFamily",
            "flashSize",
            "platformio",
            "partitionFile",
            "release",
            "parts",
        },
    )

    schema_version = _require_int(root["schemaVersion"], "schemaVersion", minimum=1)
    if schema_version != 1:
        raise DeviceProfileError(f"unsupported schemaVersion: {schema_version}")

    device_id = _require_device_id(root["id"])
    if expected_id is not None and device_id != expected_id:
        raise DeviceProfileError(
            f"profile id {device_id!r} does not match requested device {expected_id!r}"
        )

    display_name = _require_string(root["displayName"], "displayName")
    chip_family = _require_string(root["chipFamily"], "chipFamily")
    if chip_family not in _SUPPORTED_CHIP_FAMILIES:
        raise DeviceProfileError(f"unsupported chipFamily: {chip_family}")

    flash_size = _require_int(root["flashSize"], "flashSize", minimum=1)

    platformio = _require_object(root["platformio"], "platformio")
    _require_exact_keys(platformio, "platformio", {"releaseEnv"})
    release_env = _require_string(platformio["releaseEnv"], "platformio.releaseEnv")

    partition_file = _require_repo_relative_path(root["partitionFile"], "partitionFile")

    release = _require_object(root["release"], "release")
    _require_exact_keys(release, "release", {"fullImage"})
    full_image = _require_filename(release["fullImage"], "release.fullImage", suffix=".bin")

    raw_parts = root["parts"]
    if not isinstance(raw_parts, list) or not raw_parts:
        raise DeviceProfileError("parts must be a non-empty array")

    parts: list[FlashPart] = []
    names: set[str] = set()
    destinations: set[str] = set()
    for index, raw_part in enumerate(raw_parts):
        field = f"parts[{index}]"
        part = _require_object(raw_part, field)
        _require_exact_keys(part, field, {"name", "offset", "limit", "destination"})
        name = _require_string(part["name"], f"{field}.name")
        if name in names:
            raise DeviceProfileError(f"duplicate flash part name: {name}")
        names.add(name)

        offset = _require_int(part["offset"], f"{field}.offset")
        limit = _require_int(part["limit"], f"{field}.limit", minimum=1)
        if limit <= offset:
            raise DeviceProfileError(f"{field}.limit must be greater than offset")
        if limit > flash_size:
            raise DeviceProfileError(
                f"{field} ends at 0x{limit:X}, past flash size 0x{flash_size:X}"
            )

        destination = _require_filename(part["destination"], f"{field}.destination")
        if destination in destinations:
            raise DeviceProfileError(f"duplicate flash part destination: {destination}")
        destinations.add(destination)

        parts.append(
            FlashPart(
                name=name,
                offset=offset,
                limit=limit,
                destination=destination,
            )
        )

    ordered = sorted(parts, key=lambda item: (item.offset, item.limit, item.name))
    previous: FlashPart | None = None
    for part in ordered:
        if previous is not None and part.offset < previous.limit:
            raise DeviceProfileError(
                f"flash parts overlap: {previous.name} and {part.name}"
            )
        previous = part

    return DeviceProfile(
        schema_version=schema_version,
        id=device_id,
        display_name=display_name,
        chip_family=chip_family,
        flash_size=flash_size,
        platformio_release_env=release_env,
        partition_file=partition_file,
        full_image=full_image,
        parts=tuple(parts),
    )


def load_device_profile(
    *,
    device_id: str | None = None,
    profile_path: str | Path | None = None,
) -> DeviceProfile:
    if (device_id is None) == (profile_path is None):
        raise DeviceProfileError(
            "exactly one of device_id or profile_path must be provided"
        )

    expected_id: str | None = None
    if device_id is not None:
        expected_id = _require_device_id(device_id, "device_id")
        path = DEFAULT_PROFILE_DIR / f"{expected_id}.json"
    else:
        path = Path(profile_path)  # type: ignore[arg-type]

    try:
        text = path.read_text(encoding="utf-8")
    except OSError as exc:
        raise DeviceProfileError(f"cannot read device profile {path}: {exc}") from exc

    try:
        data = json.loads(text, object_pairs_hook=_reject_duplicate_keys)
    except DeviceProfileError:
        raise
    except json.JSONDecodeError as exc:
        raise DeviceProfileError(
            f"invalid JSON in device profile {path}: {exc.msg}"
        ) from exc

    profile = parse_device_profile(data, expected_id=expected_id)
    partition_path = REPOSITORY_ROOT / profile.partition_file
    if not partition_path.is_file():
        raise DeviceProfileError(
            f"partitionFile does not exist in repository: {profile.partition_file}"
        )
    return profile
