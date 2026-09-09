#pragma once

#include <Arduino.h>

#include "DeviceControlsConfig.h"

namespace move {
class MoveManager;
}

namespace hardware {

class HardwareControls {
 public:
  explicit HardwareControls(move::MoveManager& moveManager)
      : moveManager_(moveManager) {}

  void begin(Stream* log = nullptr);
  void poll(Stream* log = nullptr);

  static constexpr const char* deviceName() {
    return kDeviceControls.deviceName;
  }

 private:
  enum class LedState : uint8_t {
    Unknown,
    Idle,
    Busy,
  };

  void pollEmergencyStopButton(Stream* log);
  void updateStatusLed();
  void setLedState(LedState state);

  static constexpr uint32_t kDebounceMs = 30;
  static constexpr uint8_t kBrightness = 32;

  move::MoveManager& moveManager_;
  bool lastRawPressed_ = false;
  bool stablePressed_ = false;
  uint32_t rawChangedAtMs_ = 0;
  LedState ledState_ = LedState::Unknown;
};

}  // namespace hardware
