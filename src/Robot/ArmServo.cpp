#include "ArmServo.h"

#include "ServoConfig.h"

namespace robot {

ArmServo::ArmServo()
    : servo_(config::kArmServoPin, config::kServoFrequencyHz,
             config::kServoResolutionBits, config::kServoSpecAngle,
             config::kServoPulseMinUs, config::kServoPulseMaxUs),
      pullAngle_(config::kArmPullAngle),
      holdAngle_(config::kArmHoldAngle),
      releaseAngle_(config::kArmReleaseAngle),
      readyAngle_(config::kArmReadyAngle),
      turnSleepMs_(config::kServoTurnSleepMs),
      xSleepMs_(config::kArmXSleepMs) {}

bool ArmServo::begin(Stream& out) { return servo_.begin(out); }

bool ArmServo::calibrationValid(int pullAngle, int holdAngle, int releaseAngle,
                                int readyAngle) const {
  const int maxAngle = static_cast<int>(config::kServoSpecAngle);
  if (pullAngle < 0 || readyAngle > maxAngle) return false;
  return pullAngle <= holdAngle && holdAngle <= releaseAngle &&
         releaseAngle <= readyAngle;
}

bool ArmServo::configureCalibration(int pullAngle, int holdAngle,
                                    int releaseAngle, int readyAngle,
                                    Stream& out) {
  if (!calibrationValid(pullAngle, holdAngle, releaseAngle, readyAngle)) {
    out.printf("Invalid arm calibration: pull=%d hold=%d release=%d ready=%d (require 0 <= pull <= hold <= release <= ready <= 270)\n",
               pullAngle, holdAngle, releaseAngle, readyAngle);
    return false;
  }
  pullAngle_ = pullAngle;
  holdAngle_ = holdAngle;
  releaseAngle_ = releaseAngle;
  readyAngle_ = readyAngle;
  return true;
}

bool ArmServo::setPullAngle(int angle, Stream& out) {
  if (!calibrationValid(angle, holdAngle_, releaseAngle_, readyAngle_)) {
    out.printf("Invalid arm pull angle: %d (require 0 <= pull <= hold <= release <= ready <= 270)\n",
               angle);
    return false;
  }
  pullAngle_ = angle;
  out.printf("Arm pull angle: %d\n", pullAngle_);
  return true;
}

bool ArmServo::setHoldAngle(int angle, Stream& out) {
  if (!calibrationValid(pullAngle_, angle, releaseAngle_, readyAngle_)) {
    out.printf("Invalid arm hold angle: %d (require 0 <= pull <= hold <= release <= ready <= 270)\n",
               angle);
    return false;
  }
  holdAngle_ = angle;
  out.printf("Arm hold angle: %d\n", holdAngle_);
  return true;
}

bool ArmServo::setReleaseAngle(int angle, Stream& out) {
  if (!calibrationValid(pullAngle_, holdAngle_, angle, readyAngle_)) {
    out.printf("Invalid arm release angle: %d (require 0 <= pull <= hold <= release <= ready <= 270)\n",
               angle);
    return false;
  }
  releaseAngle_ = angle;
  out.printf("Arm release angle: %d\n", releaseAngle_);
  return true;
}

bool ArmServo::setReadyAngle(int angle, Stream& out) {
  if (!calibrationValid(pullAngle_, holdAngle_, releaseAngle_, angle)) {
    out.printf("Invalid arm ready angle: %d (require 0 <= pull <= hold <= release <= ready <= 270)\n",
               angle);
    return false;
  }
  readyAngle_ = angle;
  out.printf("Arm ready angle: %d\n", readyAngle_);
  return true;
}

bool ArmServo::setTurnSleepMs(uint32_t sleepMs, Stream& out) {
  if (sleepMs > config::kServoSleepMaxMs) {
    out.printf("Invalid servo sleep: %u ms (require 0..%u ms)\n",
               static_cast<unsigned>(sleepMs),
               static_cast<unsigned>(config::kServoSleepMaxMs));
    return false;
  }
  turnSleepMs_ = sleepMs;
  return true;
}

bool ArmServo::setXSleepMs(uint32_t sleepMs, Stream& out) {
  if (sleepMs > config::kServoSleepMaxMs) {
    out.printf("Invalid arm x sleep: %u ms (require 0..%u ms)\n",
               static_cast<unsigned>(sleepMs),
               static_cast<unsigned>(config::kServoSleepMaxMs));
    return false;
  }
  xSleepMs_ = sleepMs;
  return true;
}

bool ArmServo::turn(int angle, Stream& out) {
  if (!servo_.writeAngle(static_cast<float>(angle), &out)) return false;
  angle_ = angle;
  hold_ = angle < releaseAngle_;
  delay(turnSleepMs_);
  return true;
}

bool ArmServo::pull(Stream& out) {
  if (!turn(pullAngle_, out)) return false;
  delay(xSleepMs_);
  return true;
}

bool ArmServo::hold(Stream& out) { return turn(holdAngle_, out); }

bool ArmServo::release(Stream& out) { return turn(releaseAngle_, out); }

bool ArmServo::ready(Stream& out) { return turn(readyAngle_, out); }

bool ArmServo::turnX(uint8_t count, Stream& out) {
  if (count < 1 || count > config::kArmTurnXMaxCount) {
    out.printf("Invalid X turn count: %u\n", static_cast<unsigned>(count));
    return false;
  }

  for (uint8_t i = 0; i < count; ++i) {
    if (!isHold() && !hold(out)) return false;
    if (!pull(out)) return false;
    if (!hold(out)) return false;
  }
  return true;
}

}  // namespace robot
