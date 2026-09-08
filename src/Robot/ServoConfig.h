#pragma once

#include <Arduino.h>

namespace robot::config {

// NanoC6 Grove PORT.CUSTOM: yellow=GPIO2, white=GPIO1.
// This intentionally matches the previous AtomS3Lite Cube Scrambler wiring.
constexpr uint8_t kStandServoPin = 2;
constexpr uint8_t kArmServoPin = 1;

// GeekServo 270-degree mapping used by the existing Cube Scrambler.
constexpr uint32_t kServoFrequencyHz = 400;
constexpr uint8_t kServoResolutionBits = 12;
constexpr float kServoSpecAngle = 270.0f;
constexpr float kServoPulseMinUs = 500.0f;
constexpr float kServoPulseMaxUs = 2400.0f;
// Runtime timing values default to these values, are persisted in NVS, and
// are bounded to avoid accidentally making a robot move block for too long.
constexpr uint32_t kServoTurnSleepMs = 250;
constexpr uint32_t kServoSleepMaxMs = 5000;

// Existing project calibration (root config.ts in miso-develop/cube-scrambler).
constexpr int kStandCorrectAngle = 12;
constexpr int kStandTurnAngle = 86;
constexpr int kStandDirection = -1;
constexpr uint8_t kStandTurnDMaxCount = 2;
constexpr int kStandInitAngle =
    kStandCorrectAngle + (kStandTurnAngle * 2);  // 184 degrees

constexpr int kArmPullAngle = 173;
constexpr int kArmHoldAngle = 220;
constexpr int kArmReleaseAngle = 237;
constexpr int kArmReadyAngle = 270;
// Extra pause during each x rotation after Arm reaches the pull position.
constexpr uint32_t kArmXSleepMs = 250;
constexpr uint8_t kArmTurnXMaxCount = 3;

}  // namespace robot::config
