#include "NanoC6Controls.h"

#include <esp32-hal-rgb-led.h>

#include "../Move/MoveManager.h"

namespace hardware {

void NanoC6Controls::begin(Stream* log) {
  pinMode(kButtonPin, INPUT_PULLUP);
  lastRawPressed_ = digitalRead(kButtonPin) == LOW;
  stablePressed_ = lastRawPressed_;
  rawChangedAtMs_ = millis();

  pinMode(kRgbPowerPin, OUTPUT);
  digitalWrite(kRgbPowerPin, HIGH);
  setLedState(LedState::Idle);

  if (log) {
    log->printf("NanoC6 controls: READY button=GPIO%u rgb=GPIO%u enable=GPIO%u\n",
                static_cast<unsigned>(kButtonPin),
                static_cast<unsigned>(kRgbDataPin),
                static_cast<unsigned>(kRgbPowerPin));
    log->flush();
  }
}

void NanoC6Controls::poll(Stream* log) {
  pollEmergencyStopButton(log);
  updateStatusLed();
}

void NanoC6Controls::pollEmergencyStopButton(Stream* log) {
  const bool rawPressed = digitalRead(kButtonPin) == LOW;
  const uint32_t now = millis();

  if (rawPressed != lastRawPressed_) {
    lastRawPressed_ = rawPressed;
    rawChangedAtMs_ = now;
  }

  if (rawPressed == stablePressed_ || (now - rawChangedAtMs_) < kDebounceMs) {
    return;
  }

  stablePressed_ = rawPressed;
  if (!stablePressed_) return;

  if (moveManager_.isBusy()) {
    moveManager_.requestStop();
    if (log) {
      log->println("Emergency stop button: STOP_REQUESTED");
      log->flush();
    }
  } else if (log) {
    log->println("Emergency stop button: ignored (runner idle)");
    log->flush();
  }
}

void NanoC6Controls::updateStatusLed() {
  setLedState(moveManager_.isBusy() ? LedState::Busy : LedState::Idle);
}

void NanoC6Controls::setLedState(LedState state) {
  if (state == ledState_) return;
  ledState_ = state;

  switch (state) {
    case LedState::Idle:
      rgbLedWrite(kRgbDataPin, 0, kBrightness, 0);
      break;
    case LedState::Busy:
      rgbLedWrite(kRgbDataPin, kBrightness, 0, 0);
      break;
    case LedState::Unknown:
      rgbLedWrite(kRgbDataPin, 0, 0, 0);
      break;
  }
}

}  // namespace hardware
