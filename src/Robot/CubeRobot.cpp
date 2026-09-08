#include "CubeRobot.h"

#include <Preferences.h>
#include <cstring>

#include "ServoConfig.h"

namespace robot {
namespace {
constexpr const char* kSettingsNamespace = "cube-servo";
constexpr const char* kSettingsKey = "settings";
}

CubeRobot::Settings CubeRobot::defaultSettings() {
  return Settings{config::kStandCorrectAngle, config::kStandTurnAngle,
                  config::kArmPullAngle,      config::kArmHoldAngle,
                  config::kArmReleaseAngle,   config::kArmReadyAngle,
                  config::kServoTurnSleepMs,  config::kArmXSleepMs};
}

CubeRobot::Settings CubeRobot::currentSettings() const {
  return Settings{standServo_.correctionAngle(), standServo_.turnAngle(),
                  armServo_.pullAngle(),         armServo_.holdAngle(),
                  armServo_.releaseAngle(),      armServo_.readyAngle(),
                  standServo_.turnSleepMs(),     armServo_.xSleepMs()};
}

bool CubeRobot::applySettings(const Settings& settings, Stream& out) {
  if (settings.servoSleepMs > config::kServoSleepMaxMs ||
      settings.armXSleepMs > config::kServoSleepMaxMs) {
    out.printf("Invalid servo timing: servo-sleep=%u arm-x-sleep=%u (require 0..%u ms)\n",
               static_cast<unsigned>(settings.servoSleepMs),
               static_cast<unsigned>(settings.armXSleepMs),
               static_cast<unsigned>(config::kServoSleepMaxMs));
    return false;
  }

  if (!standServo_.configureCalibration(settings.standCorrection,
                                        settings.standTurn, out)) {
    return false;
  }
  if (!armServo_.configureCalibration(settings.armPull, settings.armHold,
                                      settings.armRelease, settings.armReady,
                                      out)) {
    return false;
  }
  if (!standServo_.setTurnSleepMs(settings.servoSleepMs, out)) return false;
  if (!armServo_.setTurnSleepMs(settings.servoSleepMs, out)) return false;
  if (!armServo_.setXSleepMs(settings.armXSleepMs, out)) return false;
  return true;
}

bool CubeRobot::ensureSettingsLoaded(Stream& out) {
  if (settingsLoaded_) return true;

  Settings settings = defaultSettings();
  bool storedSettingsFound = false;
  Preferences preferences;
  if (preferences.begin(kSettingsNamespace, true)) {
    if (preferences.getBytesLength(kSettingsKey) == sizeof(Settings)) {
      storedSettingsFound =
          preferences.getBytes(kSettingsKey, &settings, sizeof(settings)) ==
          sizeof(settings);
    }
    preferences.end();
  } else {
    out.println("Servo settings NVS unavailable; using defaults");
  }

  if (!applySettings(settings, out)) {
    out.println("Stored servo settings invalid; falling back to defaults");
    settings = defaultSettings();
    storedSettingsFound = false;
    if (!applySettings(settings, out)) {
      out.println("Default servo settings rejected");
      out.flush();
      return false;
    }
  }

  settingsLoaded_ = true;
  out.printf("Servo settings loaded: %s\n",
             storedSettingsFound ? "NVS" : "defaults");
  out.flush();
  return true;
}

bool CubeRobot::persistSettings(const Settings& settings, Stream& out) const {
  Preferences preferences;
  if (!preferences.begin(kSettingsNamespace, false)) {
    out.println("Servo settings save failed: NVS unavailable");
    return false;
  }

  const bool saved =
      preferences.putBytes(kSettingsKey, &settings, sizeof(settings)) ==
      sizeof(settings);
  preferences.end();

  if (!saved) out.println("Servo settings save failed: NVS write error");
  return saved;
}

bool CubeRobot::begin(Stream& out) {
  if (!ensureSettingsLoaded(out)) return false;
  if (attached_) return true;

  out.println();
  out.println("Servo attach BEGIN");
  if (!standServo_.begin(out)) return false;
  if (!armServo_.begin(out)) {
    standServo_.detach();
    return false;
  }
  attached_ = true;
  out.println("Servo attach OK");
  out.flush();
  return true;
}

bool CubeRobot::init(Stream& out) {
  if (!begin(out)) return false;

  out.println();
  out.println("CubeRobot init BEGIN");
  // Move the arm clear first, then center the stand. The old PC implementation
  // sent these concurrently; ordering them this way is safer for direct PWM.
  if (!armServo_.ready(out)) return false;
  if (!standServo_.init(out)) return false;
  ready_ = true;
  out.println("CubeRobot init OK");
  out.flush();
  return true;
}

void CubeRobot::stop(Stream& out) {
  standServo_.detach();
  armServo_.detach();
  attached_ = false;
  ready_ = false;
  out.println("Servo PWM detached: GPIO2/GPIO1 released");
  out.flush();
}

bool CubeRobot::requireReady(Stream& out) const {
  if (ready_) return true;
  out.println("CubeRobot not initialized. Run servo-init first.");
  out.flush();
  return false;
}

bool CubeRobot::park(Stream& out) {
  if (!requireReady(out)) return false;
  return armServo_.ready(out);
}

bool CubeRobot::d(uint8_t count, Stream& out) {
  if (!requireReady(out)) return false;
  if (!armServo_.isHold() && !armServo_.hold(out)) return false;
  return standServo_.turnD(count, out);
}

bool CubeRobot::dp(uint8_t count, Stream& out) {
  if (!requireReady(out)) return false;
  if (!armServo_.isHold() && !armServo_.hold(out)) return false;
  return standServo_.turnDP(count, out);
}

bool CubeRobot::x(uint8_t count, Stream& out) {
  if (!requireReady(out)) return false;
  return armServo_.turnX(count, out);
}

bool CubeRobot::y(uint8_t count, Stream& out) {
  if (!requireReady(out)) return false;
  if (!armServo_.isRelease() && !armServo_.release(out)) return false;
  return standServo_.turnDP(count, out);
}

bool CubeRobot::yp(uint8_t count, Stream& out) {
  if (!requireReady(out)) return false;
  if (!armServo_.isRelease() && !armServo_.release(out)) return false;
  return standServo_.turnD(count, out);
}

void CubeRobot::printStatus(Stream& out) const {
  out.println();
  out.println("Servo status");
  out.printf("Attached: %s\n", attached_ ? "YES" : "NO");
  out.printf("Robot initialized: %s\n", ready_ ? "YES" : "NO");
  out.printf("Stand GPIO: %u\n", static_cast<unsigned>(config::kStandServoPin));
  out.printf("Stand angle: %d\n", standServo_.angle());
  out.printf("Arm GPIO: %u\n", static_cast<unsigned>(config::kArmServoPin));
  out.printf("Arm angle: %d\n", armServo_.angle());
  const char* armState = armServo_.angle() == armServo_.readyAngle()
                             ? "ready"
                             : (armServo_.isHold() ? "hold" : "release");
  out.printf("Arm state: %s\n", armState);
  out.flush();
}

void CubeRobot::printCalibration(Stream& out) {
  if (!ensureSettingsLoaded(out)) return;

  out.println();
  out.println("Servo settings (persistent NVS)");
  out.printf("stand-correct: %d deg\n", standServo_.correctionAngle());
  out.printf("stand-turn: %d deg\n", standServo_.turnAngle());
  out.printf("stand-init: %d deg (derived)\n", standServo_.initAngle());
  out.printf("arm-pull: %d deg\n", armServo_.pullAngle());
  out.printf("arm-hold: %d deg\n", armServo_.holdAngle());
  out.printf("arm-release: %d deg\n", armServo_.releaseAngle());
  out.printf("arm-ready: %d deg\n", armServo_.readyAngle());
  out.printf("servo-sleep: %u ms (Stand/Arm common)\n",
             static_cast<unsigned>(standServo_.turnSleepMs()));
  out.printf("arm-x-sleep: %u ms (extra pause at x pull position)\n",
             static_cast<unsigned>(armServo_.xSleepMs()));
  out.flush();
}

bool CubeRobot::setCalibration(const char* name, int value, Stream& out) {
  if (!name || name[0] == '\0') return false;
  if (!ensureSettingsLoaded(out)) return false;

  const Settings before = currentSettings();
  Settings target = before;

  if (std::strcmp(name, "stand-correct") == 0) {
    target.standCorrection = value;
  } else if (std::strcmp(name, "stand-turn") == 0) {
    target.standTurn = value;
  } else if (std::strcmp(name, "arm-pull") == 0) {
    target.armPull = value;
  } else if (std::strcmp(name, "arm-hold") == 0) {
    target.armHold = value;
  } else if (std::strcmp(name, "arm-release") == 0) {
    target.armRelease = value;
  } else if (std::strcmp(name, "arm-ready") == 0) {
    target.armReady = value;
  } else if (std::strcmp(name, "servo-sleep") == 0) {
    if (value < 0) {
      out.println("servo-sleep must be 0 or greater");
      return false;
    }
    target.servoSleepMs = static_cast<uint32_t>(value);
  } else if (std::strcmp(name, "arm-x-sleep") == 0) {
    if (value < 0) {
      out.println("arm-x-sleep must be 0 or greater");
      return false;
    }
    target.armXSleepMs = static_cast<uint32_t>(value);
  } else {
    out.printf("Unknown servo setting key: %s\n", name);
    return false;
  }

  if (!applySettings(target, out)) {
    applySettings(before, out);
    out.flush();
    return false;
  }

  if (!persistSettings(target, out)) {
    applySettings(before, out);
    out.println("Servo setting rolled back because persistence failed");
    out.flush();
    return false;
  }

  out.printf("Servo setting saved: %s=%d\n", name, value);
  out.println("Setting is effective immediately and persisted to NVS.");
  out.flush();
  return true;
}

}  // namespace robot
