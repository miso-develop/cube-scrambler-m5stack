#pragma once

#include <Arduino.h>
#include <cstdint>

namespace hardware {

struct DeviceControlsConfig {
  const char* deviceName;
  uint8_t stopButtonPin;
  uint8_t stopButtonMode;
  uint8_t stopActiveLevel;
  uint8_t rgbDataPin;
  int16_t rgbPowerPin;
  uint8_t rgbPowerEnabledLevel;
};

#if defined(CUBE_DEVICE_NANOC6) && defined(CUBE_DEVICE_ATOMS3_LITE)
#error "Exactly one Cube Scrambler device must be selected"
#elif defined(CUBE_DEVICE_NANOC6)
inline constexpr DeviceControlsConfig kDeviceControls{
    "NanoC6",
    9,
    INPUT_PULLUP,
    LOW,
    20,
    19,
    HIGH,
};
static_assert(kDeviceControls.stopButtonPin == 9);
static_assert(kDeviceControls.stopActiveLevel == LOW);
static_assert(kDeviceControls.rgbDataPin == 20);
static_assert(kDeviceControls.rgbPowerPin == 19);
#elif defined(CUBE_DEVICE_ATOMS3_LITE)
inline constexpr DeviceControlsConfig kDeviceControls{
    "AtomS3 Lite",
    41,
    INPUT_PULLUP,
    LOW,
    35,
    -1,
    HIGH,
};
static_assert(kDeviceControls.stopButtonPin == 41);
static_assert(kDeviceControls.stopActiveLevel == LOW);
static_assert(kDeviceControls.rgbDataPin == 35);
static_assert(kDeviceControls.rgbPowerPin < 0);
#else
#error "Unsupported Cube Scrambler device: select exactly one device build target"
#endif

inline constexpr bool hasRgbPowerEnable() {
  return kDeviceControls.rgbPowerPin >= 0;
}

}  // namespace hardware
