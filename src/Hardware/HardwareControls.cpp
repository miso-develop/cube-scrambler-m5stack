#include "HardwareControls.h"

#include <esp32-hal-rgb-led.h>

#include "../Move/MoveManager.h"

namespace hardware {

void HardwareControls::begin(Stream* log) {
  pinMode(kDeviceControls.stopButtonPin, kDeviceControls.stopButtonMode);
  lastRawPressed_ =
      digitalRead(kDeviceControls.stopButtonPin) == kDeviceControls.stopActiveLevel;
  stablePressed_ = lastRawPressed_;
  rawChangedAtMs_ = millis();

  if (hasRgbPowerEnable()) {
    const auto powerPin = static_cast<uint8_t>(kDeviceControls.rgbPowerPin);
    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin, kDeviceControls.rgbPowerEnabledLevel);
  }
  setLedState(LedState::Idle);

  if (log) {
    log->printf("%s controls: READY button=GPIO%u rgb=GPIO%u",
                kDeviceControls.deviceName,
                static_cast<unsigned>(kDeviceControls.stopButtonPin),
                static_cast<unsigned>(kDeviceControls.rgbDataPin));
    if (hasRgbPowerEnable()) {
      log->printf(" enable=GPIO%u",
                  static_cast<unsigned>(kDeviceControls.rgbPowerPin));
    }
    log->println();
    log->flush();
  }
}

void HardwareControls::poll(Stream* log) {
  pollEmergencyStopButton(log);
  updateStatusLed();
}

void HardwareControls::pollEmergencyStopButton(Stream* log) {
  const bool rawPressed =
      digitalRead(kDeviceControls.stopButtonPin) == kDeviceControls.stopActiveLevel;
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

void HardwareControls::updateStatusLed() {
  setLedState(moveManager_.isBusy() ? LedState::Busy : LedState::Idle);
}

void HardwareControls::setLedState(LedState state) {
  if (state == ledState_) return;
  ledState_ = state;

  switch (state) {
    case LedState::Idle:
      rgbLedWrite(kDeviceControls.rgbDataPin, 0, kBrightness, 0);
      break;
    case LedState::Busy:
      rgbLedWrite(kDeviceControls.rgbDataPin, kBrightness, 0, 0);
      break;
    case LedState::Unknown:
      rgbLedWrite(kDeviceControls.rgbDataPin, 0, 0, 0);
      break;
  }
}

}  // namespace hardware
