#!/usr/bin/env python3
from __future__ import annotations

import copy
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from build_release_bundle import build_bundle, validate_sources
from check_flash_layout import check_profile_partition_contract, load_partitions
from device_profiles import load_device_profile, parse_device_profile


class ProfileDrivenReleaseTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.profile = load_device_profile(device_id="m5stack-nanoc6")
        cls.partitions = load_partitions(Path(cls.profile.partition_file))
        cls.atoms3_lite_profile = load_device_profile(
            device_id="m5stack-atoms3-lite"
        )
        cls.atoms3_lite_partitions = load_partitions(
            Path(cls.atoms3_lite_profile.partition_file)
        )

    def test_partition_contract_matches_nanoc6_profile(self) -> None:
        check_profile_partition_contract(self.partitions, self.profile)

    def test_partition_contract_matches_atoms3_lite_profile(self) -> None:
        check_profile_partition_contract(
            self.atoms3_lite_partitions,
            self.atoms3_lite_profile,
        )

    def test_partition_contract_rejects_profile_drift(self) -> None:
        data = self._profile_dict()
        solver = next(part for part in data["parts"] if part["name"] == "solver")
        solver["offset"] += 0x1000
        drifted = parse_device_profile(data)
        with self.assertRaisesRegex(ValueError, "does not match profile part solver"):
            check_profile_partition_contract(self.partitions, drifted)

    def test_atoms3_lite_partition_contract_rejects_layout_drift(self) -> None:
        drifted = copy.deepcopy(self.atoms3_lite_partitions)
        solver = next(part for part in drifted if part["name"] == "solver")
        solver["size"] = int(solver["size"]) - 0x1000
        with self.assertRaisesRegex(ValueError, "does not match profile part solver"):
            check_profile_partition_contract(drifted, self.atoms3_lite_profile)

    def test_source_preflight_rejects_oversized_image(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sources: dict[str, Path] = {}
            for part in self.profile.parts:
                path = root / part.destination
                size = 1
                if part.name == "firmware":
                    size = part.limit - part.offset + 1
                path.write_bytes(b"x" * size)
                sources[part.name] = path
            with self.assertRaisesRegex(ValueError, "exceeds reserved capacity"):
                validate_sources(self.profile, sources)

    def test_nanoc6_bundle_preserves_release_contract(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            build_dir = root / "build"
            build_dir.mkdir()
            solver = root / "solver.bin"
            web = root / "web.bin"
            ca_cert = root / "ca.crt"
            boot_app0 = root / "boot_app0.bin"
            output = root / "release"
            installer = root / "web_installer"
            installer.mkdir()
            (installer / "index.html").write_text("<html></html>", encoding="utf-8")
            (installer / "installer.js").write_text(
                "console.log('ok');", encoding="utf-8"
            )
            ca_cert.write_text("test-ca", encoding="utf-8")

            images = {
                build_dir / "bootloader.bin": 0x1000,
                build_dir / "partitions.bin": 0x1000,
                build_dir / "firmware.bin": 0x2000,
                boot_app0: 0x1000,
                solver: 0x2000,
                web: 0x2000,
            }
            for path, size in images.items():
                path.write_bytes(b"\xA5" * size)

            with patch("build_release_bundle.WEB_INSTALLER_SOURCE", installer):
                merged = build_bundle(
                    profile=self.profile,
                    build_dir=build_dir,
                    solver=solver,
                    web=web,
                    ca_cert=ca_cert,
                    boot_app0=boot_app0,
                    output=output,
                    version="test",
                    repository="https://github.com/miso-develop/cube-scrambler-m5stack",
                )

            self.assertEqual(merged.name, "cube-scrambler-nanoc6-full.bin")
            self.assertEqual(merged.stat().st_size, 0x400000)
            manifest = json.loads(
                (output / "web-installer" / "manifest.json").read_text(
                    encoding="utf-8"
                )
            )
            release = json.loads(
                (output / "release.json").read_text(encoding="utf-8")
            )
            self.assertEqual(manifest["builds"][0]["chipFamily"], "ESP32-C6")
            self.assertEqual(
                manifest["builds"][0]["parts"],
                [{"path": "cube-scrambler-nanoc6-full.bin", "offset": 0}],
            )
            self.assertEqual(release["deviceId"], "m5stack-nanoc6")
            self.assertEqual(release["flashSize"], 0x400000)
            self.assertEqual(
                release["platformioReleaseEnv"], "m5stack-nanoc6-release"
            )
            records = {part["name"]: part for part in release["parts"]}
            self.assertEqual(records["solver"]["offset"], 0x2B0000)
            self.assertEqual(records["web"]["offset"], 0x3C0000)

    def _profile_dict(self) -> dict[str, object]:
        path = Path("config/devices/m5stack-nanoc6.json")
        return copy.deepcopy(json.loads(path.read_text(encoding="utf-8")))


if __name__ == "__main__":
    unittest.main()
