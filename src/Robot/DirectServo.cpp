#include "DirectServo.h"

#include <cmath>

namespace robot {

DirectServo::DirectServo(uint8_t pin, uint32_t frequencyHz,
                         uint8_t resolutionBits, float specAngle,
                         float pulseMinUs, float pulseMaxUs)
    : pin_(pin),
      frequencyHz_(frequencyHz),
      resolutionBits_(resolutionBits),
      specAngle_(specAngle),
      pulseMinUs_(pulseMinUs),
      pulseMaxUs_(pulseMaxUs) {}

bool DirectServo::begin(Stream& out) {
  if (attached_) return true;
  if (!ledcAttach(pin_, frequencyHz_, resolutionBits_)) {
    out.printf("Servo attach FAILED: GPIO%u\n", static_cast<unsigned>(pin_));
    return false;
  }
  attached_ = true;
  out.printf("Servo attach OK: GPIO%u @ %lu Hz / %u bit\n",
             static_cast<unsigned>(pin_),
             static_cast<unsigned long>(frequencyHz_),
             static_cast<unsigned>(resolutionBits_));
  return true;
}

uint32_t DirectServo::dutyForAngle(float angle) const {
  const float ratio = angle / specAngle_;
  const float pulseUs = ratio * (pulseMaxUs_ - pulseMinUs_) + pulseMinUs_;
  const float periodUs = 1000000.0f / static_cast<float>(frequencyHz_);
  const uint32_t resolution = 1UL << resolutionBits_;
  return static_cast<uint32_t>(std::lround((pulseUs / periodUs) * resolution));
}

bool DirectServo::writeAngle(float angle, Stream* out) {
  if (!attached_) {
    if (out) out->printf("Servo GPIO%u not attached\n", static_cast<unsigned>(pin_));
    return false;
  }
  if (angle < 0.0f || angle > specAngle_) {
    if (out) {
      out->printf("Servo GPIO%u invalid angle: %.1f\n",
                  static_cast<unsigned>(pin_), angle);
    }
    return false;
  }

  const uint32_t duty = dutyForAngle(angle);
  if (!ledcWrite(pin_, duty)) {
    if (out) out->printf("Servo GPIO%u LEDC write FAILED\n", static_cast<unsigned>(pin_));
    return false;
  }
  angle_ = angle;
  if (out) {
    out->printf("Servo GPIO%u -> %.1f deg (duty=%lu)\n",
                static_cast<unsigned>(pin_), angle,
                static_cast<unsigned long>(duty));
  }
  return true;
}

void DirectServo::detach() {
  if (!attached_) return;
  ledcDetach(pin_);
  attached_ = false;
}

}  // namespace robot
