#pragma once

#include <Arduino.h>

namespace move {
class MoveManager;
}

namespace hardware {

class NanoC6Controls {
 public:
  explicit NanoC6Controls(move::MoveManager& moveManager)
      : moveManager_(moveManager) {}

  void begin(Stream* log = nullptr);
  void poll(Stream* log = nullptr);

 private:
  enum class LedState : uint8_t {
    Unknown,
    Idle,
    Busy,
  };

  void pollEmergencyStopButton(Stream* log);
  void updateStatusLed();
  void setLedState(LedState state);

  static constexpr uint8_t kButtonPin = 9;
  static constexpr uint8_t kRgbDataPin = 20;
  static constexpr uint8_t kRgbPowerPin = 19;
  static constexpr uint32_t kDebounceMs = 30;
  static constexpr uint8_t kBrightness = 32;

  move::MoveManager& moveManager_;
  bool lastRawPressed_ = false;
  bool stablePressed_ = false;
  uint32_t rawChangedAtMs_ = 0;
  LedState ledState_ = LedState::Unknown;
};

}  // namespace hardware
