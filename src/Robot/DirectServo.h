#pragma once

#include <Arduino.h>

namespace robot {

class DirectServo {
 public:
  DirectServo(uint8_t pin, uint32_t frequencyHz, uint8_t resolutionBits,
              float specAngle, float pulseMinUs, float pulseMaxUs);

  bool begin(Stream& out);
  bool attached() const { return attached_; }
  bool writeAngle(float angle, Stream* out = nullptr);
  void detach();

  uint8_t pin() const { return pin_; }
  float angle() const { return angle_; }
  uint32_t dutyForAngle(float angle) const;

 private:
  uint8_t pin_;
  uint32_t frequencyHz_;
  uint8_t resolutionBits_;
  float specAngle_;
  float pulseMinUs_;
  float pulseMaxUs_;
  float angle_ = 0.0f;
  bool attached_ = false;
};

}  // namespace robot
