#pragma once

#include <Arduino.h>

#include "DirectServo.h"

namespace robot {

class ArmServo {
 public:
  ArmServo();

  bool begin(Stream& out);
  void detach() { servo_.detach(); }
  bool hold(Stream& out);
  bool release(Stream& out);
  bool ready(Stream& out);
  bool turnX(uint8_t count, Stream& out);

  bool configureCalibration(int pullAngle, int holdAngle, int releaseAngle,
                            int readyAngle, Stream& out);
  bool setPullAngle(int angle, Stream& out);
  bool setHoldAngle(int angle, Stream& out);
  bool setReleaseAngle(int angle, Stream& out);
  bool setReadyAngle(int angle, Stream& out);
  bool setTurnSleepMs(uint32_t sleepMs, Stream& out);
  bool setXSleepMs(uint32_t sleepMs, Stream& out);

  bool attached() const { return servo_.attached(); }
  bool isHold() const { return hold_; }
  bool isRelease() const { return !hold_; }
  int angle() const { return angle_; }
  int pullAngle() const { return pullAngle_; }
  int holdAngle() const { return holdAngle_; }
  int releaseAngle() const { return releaseAngle_; }
  int readyAngle() const { return readyAngle_; }
  uint32_t turnSleepMs() const { return turnSleepMs_; }
  uint32_t xSleepMs() const { return xSleepMs_; }

 private:
  bool calibrationValid(int pullAngle, int holdAngle, int releaseAngle,
                        int readyAngle) const;
  bool turn(int angle, Stream& out);
  bool pull(Stream& out);

  DirectServo servo_;
  bool hold_ = false;
  int angle_ = 0;
  int pullAngle_;
  int holdAngle_;
  int releaseAngle_;
  int readyAngle_;
  uint32_t turnSleepMs_;
  uint32_t xSleepMs_;
};

}  // namespace robot
