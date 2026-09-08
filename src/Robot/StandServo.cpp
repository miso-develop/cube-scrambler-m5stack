#include "StandServo.h"

#include <cstdlib>

#include "ServoConfig.h"

namespace robot {

StandServo::StandServo()
    : servo_(config::kStandServoPin, config::kServoFrequencyHz,
             config::kServoResolutionBits, config::kServoSpecAngle,
             config::kServoPulseMinUs, config::kServoPulseMaxUs),
      angle_(config::kStandInitAngle),
      correctionAngle_(config::kStandCorrectAngle),
      turnAngle_(config::kStandTurnAngle),
      turnSleepMs_(config::kServoTurnSleepMs) {}

bool StandServo::begin(Stream& out) { return servo_.begin(out); }

bool StandServo::calibrationValid(int correctionAngle, int turnAngle) const {
  if (correctionAngle < 0 || turnAngle <= 0) return false;
  const int init = correctionAngle + (turnAngle * 2);
  return init >= 0 && init <= static_cast<int>(config::kServoSpecAngle);
}

bool StandServo::configureCalibration(int correctionAngle, int turnAngle,
                                     Stream& out) {
  if (!calibrationValid(correctionAngle, turnAngle)) {
    out.printf("Invalid stand calibration: correct=%d turn=%d (derived init must be 0..270)\n",
               correctionAngle, turnAngle);
    return false;
  }
  correctionAngle_ = correctionAngle;
  turnAngle_ = turnAngle;
  return true;
}

bool StandServo::setCorrectionAngle(int angle, Stream& out) {
  if (!calibrationValid(angle, turnAngle_)) {
    out.printf("Invalid stand correction angle: %d (derived init must be 0..270)\n",
               angle);
    return false;
  }
  correctionAngle_ = angle;
  out.printf("Stand correction angle: %d; derived init: %d\n",
             correctionAngle_, initAngle());
  return true;
}

bool StandServo::setTurnAngle(int angle, Stream& out) {
  if (!calibrationValid(correctionAngle_, angle)) {
    out.printf("Invalid stand turn angle: %d (derived init must be 0..270)\n",
               angle);
    return false;
  }
  turnAngle_ = angle;
  out.printf("Stand turn angle: %d; derived init: %d\n", turnAngle_, initAngle());
  return true;
}

bool StandServo::setTurnSleepMs(uint32_t sleepMs, Stream& out) {
  if (sleepMs > config::kServoSleepMaxMs) {
    out.printf("Invalid servo sleep: %u ms (require 0..%u ms)\n",
               static_cast<unsigned>(sleepMs),
               static_cast<unsigned>(config::kServoSleepMaxMs));
    return false;
  }
  turnSleepMs_ = sleepMs;
  return true;
}

bool StandServo::turn(int angle, Stream& out) {
  if (!servo_.writeAngle(static_cast<float>(angle), &out)) return false;
  angle_ = angle;
  delay(turnSleepMs_);
  return true;
}

bool StandServo::init(Stream& out) {
  out.printf("Stand init angle: %d\n", initAngle());
  return turn(initAngle(), out);
}

bool StandServo::turn90(int count, int direction, Stream& out) {
  const int gearCorrectionDirection = direction * config::kStandDirection;
  const int nextAngle = angle_ + (turnAngle_ * gearCorrectionDirection * count);

  if (nextAngle >= 0 && nextAngle <= static_cast<int>(config::kServoSpecAngle)) {
    if (!turn(nextAngle, out)) return false;
    const int extraCount = std::abs(count) - 1;
    if (extraCount > 0) delay(200U * static_cast<uint32_t>(extraCount));
    return true;
  }

  // Preserve the original StandServo wrap behavior. One full mechanical
  // cycle is STAND_TURN_D_MAX_COUNT * 2 quarter-turns.
  const int turnMaxCount = config::kStandTurnDMaxCount * 2;
  if (std::abs(count - turnMaxCount) > turnMaxCount) {
    out.println("Stand rotation wrap failed: max rotation count exceeded");
    return false;
  }
  return turn90(count - turnMaxCount, direction, out);
}

bool StandServo::turnD(uint8_t count, Stream& out) {
  if (count < 1 || count > config::kStandTurnDMaxCount) {
    out.printf("Invalid D turn count: %u\n", static_cast<unsigned>(count));
    return false;
  }
  return turn90(count, -1, out);
}

bool StandServo::turnDP(uint8_t count, Stream& out) {
  if (count < 1 || count > config::kStandTurnDMaxCount) {
    out.printf("Invalid D' turn count: %u\n", static_cast<unsigned>(count));
    return false;
  }
  return turn90(count, 1, out);
}

}  // namespace robot
