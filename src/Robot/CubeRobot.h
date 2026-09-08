#pragma once

#include <Arduino.h>

#include "ArmServo.h"
#include "StandServo.h"

namespace robot {

class CubeRobot {
 public:
  bool begin(Stream& out);
  bool init(Stream& out);
  void stop(Stream& out);
  bool ready() const { return ready_; }
  bool park(Stream& out);

  bool d(uint8_t count, Stream& out);
  bool dp(uint8_t count, Stream& out);
  bool x(uint8_t count, Stream& out);
  bool y(uint8_t count, Stream& out);
  bool yp(uint8_t count, Stream& out);

  void printStatus(Stream& out) const;
  void printCalibration(Stream& out);
  bool setCalibration(const char* name, int value, Stream& out);

  int standCorrectionAngle() const { return standServo_.correctionAngle(); }
  int standTurnAngle() const { return standServo_.turnAngle(); }
  int armPullAngle() const { return armServo_.pullAngle(); }
  int armHoldAngle() const { return armServo_.holdAngle(); }
  int armReleaseAngle() const { return armServo_.releaseAngle(); }
  int armReadyAngle() const { return armServo_.readyAngle(); }
  uint32_t servoSleepMs() const { return standServo_.turnSleepMs(); }
  uint32_t armXSleepMs() const { return armServo_.xSleepMs(); }

 private:
  struct Settings {
    int standCorrection;
    int standTurn;
    int armPull;
    int armHold;
    int armRelease;
    int armReady;
    uint32_t servoSleepMs;
    uint32_t armXSleepMs;
  };

  static Settings defaultSettings();
  Settings currentSettings() const;
  bool ensureSettingsLoaded(Stream& out);
  bool applySettings(const Settings& settings, Stream& out);
  bool persistSettings(const Settings& settings, Stream& out) const;
  bool requireReady(Stream& out) const;

  StandServo standServo_;
  ArmServo armServo_;
  bool attached_ = false;
  bool ready_ = false;
  bool settingsLoaded_ = false;
};

}  // namespace robot
