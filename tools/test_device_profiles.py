#!/usr/bin/env python3
from __future__ import annotations

import copy
import json
import tempfile
import unittest
from pathlib import Path

from device_profiles import DeviceProfileError, load_device_profile, parse_device_profile


class DeviceProfilesTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.nanoc6 = load_device_profile(device_id="m5stack-nanoc6")
        cls.atoms3_lite = load_device_profile(device_id="m5stack-atoms3-lite")

    def test_nanoc6_profile_matches_release_baseline(self) -> None:
        self.assertEqual(self.nanoc6.id, "m5stack-nanoc6")
        self.assertEqual(self.nanoc6.display_name, "M5Stack NanoC6")
        self.assertEqual(self.nanoc6.chip_family, "ESP32-C6")
        self.assertEqual(self.nanoc6.flash_size, 0x400000)
        self.assertEqual(
            self.nanoc6.platformio_release_env, "m5stack-nanoc6-release"
        )
        self.assertEqual(
            self.nanoc6.partition_file, "partitions/cube_scrambler_4mb.csv"
        )
        self.assertEqual(
            self.nanoc6.full_image, "cube-scrambler-nanoc6-full.bin"
        )
        self.assertEqual(self.nanoc6.part("solver").offset, 0x2B0000)
        self.assertEqual(self.nanoc6.part("web").offset, 0x3C0000)
        self.assertEqual(self.nanoc6.part("web").limit, 0x400000)

    def test_atoms3_lite_profile_matches_decision(self) -> None:
        self.assertEqual(self.atoms3_lite.id, "m5stack-atoms3-lite")
        self.assertEqual(self.atoms3_lite.display_name, "M5Stack AtomS3 Lite")
        self.assertEqual(self.atoms3_lite.chip_family, "ESP32-S3")
        self.assertEqual(self.atoms3_lite.flash_size, 0x800000)
        self.assertEqual(
            self.atoms3_lite.platformio_release_env,
            "m5stack-atoms3-lite-release",
        )
        self.assertEqual(
            self.atoms3_lite.partition_file,
            "partitions/cube_scrambler_atoms3_lite_8mb.csv",
        )
        self.assertEqual(
            self.atoms3_lite.full_image,
            "cube-scrambler-atoms3-lite-full.bin",
        )
        self.assertEqual(self.atoms3_lite.part("firmware").offset, 0x10000)
        self.assertEqual(self.atoms3_lite.part("firmware").limit, 0x290000)
        self.assertEqual(self.atoms3_lite.part("solver").offset, 0x510000)
        self.assertEqual(self.atoms3_lite.part("solver").limit, 0x690000)
        self.assertEqual(self.atoms3_lite.part("web").offset, 0x690000)
        self.assertEqual(self.atoms3_lite.part("web").limit, 0x800000)

    def test_explicit_profile_path_loads_same_profile(self) -> None:
        profile_path = (
            Path(__file__).resolve().parents[1]
            / "config"
            / "devices"
            / "m5stack-nanoc6.json"
        )
        loaded = load_device_profile(profile_path=profile_path)
        self.assertEqual(loaded, self.nanoc6)

    def test_missing_required_field_is_rejected(self) -> None:
        data = self._profile_dict()
        del data["chipFamily"]
        with self.assertRaisesRegex(DeviceProfileError, "missing required fields"):
            parse_device_profile(data)

    def test_unknown_field_is_rejected(self) -> None:
        data = self._profile_dict()
        data["futureGuess"] = True
        with self.assertRaisesRegex(DeviceProfileError, "unsupported fields"):
            parse_device_profile(data)

    def test_overlapping_parts_are_rejected(self) -> None:
        data = self._profile_dict()
        data["parts"][1]["offset"] = 0x7000
        with self.assertRaisesRegex(DeviceProfileError, "overlap"):
            parse_device_profile(data)

    def test_part_past_flash_end_is_rejected(self) -> None:
        data = self._profile_dict()
        data["parts"][-1]["limit"] = 0x400001
        with self.assertRaisesRegex(DeviceProfileError, "past flash size"):
            parse_device_profile(data)

    def test_negative_offset_is_rejected(self) -> None:
        data = self._profile_dict()
        data["parts"][0]["offset"] = -1
        with self.assertRaisesRegex(DeviceProfileError, "integer >= 0"):
            parse_device_profile(data)

    def test_noncanonical_partition_path_is_rejected(self) -> None:
        data = self._profile_dict()
        data["partitionFile"] = "partitions//cube_scrambler_4mb.csv"
        with self.assertRaisesRegex(DeviceProfileError, "normalized repository-relative"):
            parse_device_profile(data)

    def test_duplicate_json_key_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "duplicate.json"
            path.write_text(
                '{"schemaVersion": 1, "schemaVersion": 1}',
                encoding="utf-8",
            )
            with self.assertRaisesRegex(DeviceProfileError, "duplicate JSON key"):
                load_device_profile(profile_path=path)

    def test_unsafe_device_id_is_rejected_before_path_resolution(self) -> None:
        with self.assertRaisesRegex(DeviceProfileError, "path-safe hyphens"):
            load_device_profile(device_id="../m5stack-nanoc6")

    def test_selection_is_explicit_and_unambiguous(self) -> None:
        with self.assertRaisesRegex(DeviceProfileError, "exactly one"):
            load_device_profile()
        with self.assertRaisesRegex(DeviceProfileError, "exactly one"):
            load_device_profile(
                device_id="m5stack-nanoc6",
                profile_path="config/devices/m5stack-nanoc6.json",
            )

    def _profile_dict(self) -> dict[str, object]:
        path = (
            Path(__file__).resolve().parents[1]
            / "config"
            / "devices"
            / "m5stack-nanoc6.json"
        )
        return copy.deepcopy(json.loads(path.read_text(encoding="utf-8")))


if __name__ == "__main__":
    unittest.main()
