#pragma once

#include <Arduino.h>

#include "DirectServo.h"

namespace robot {

class StandServo {
 public:
  StandServo();

  bool begin(Stream& out);
  void detach() { servo_.detach(); }
  bool init(Stream& out);
  bool turnD(uint8_t count, Stream& out);
  bool turnDP(uint8_t count, Stream& out);

  bool configureCalibration(int correctionAngle, int turnAngle, Stream& out);
  bool setCorrectionAngle(int angle, Stream& out);
  bool setTurnAngle(int angle, Stream& out);
  bool setTurnSleepMs(uint32_t sleepMs, Stream& out);

  bool attached() const { return servo_.attached(); }
  int angle() const { return angle_; }
  int correctionAngle() const { return correctionAngle_; }
  int turnAngle() const { return turnAngle_; }
  int initAngle() const { return correctionAngle_ + (turnAngle_ * 2); }
  uint32_t turnSleepMs() const { return turnSleepMs_; }

 private:
  bool calibrationValid(int correctionAngle, int turnAngle) const;
  bool turn(int angle, Stream& out);
  bool turn90(int count, int direction, Stream& out);

  DirectServo servo_;
  int angle_;
  int correctionAngle_;
  int turnAngle_;
  uint32_t turnSleepMs_;
};

}  // namespace robot
