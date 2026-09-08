#pragma once

#include <Arduino.h>

namespace network {

enum class WifiMode : uint8_t {
  Station,
  AccessPoint,
};

struct WifiSettings {
  WifiMode mode = WifiMode::Station;
  String stationSsid;
  String stationPassword;
  bool stationCredentialsStored = false;
};

class WifiSettingsStore {
 public:
  static WifiSettings load(const char* fallbackSsid = nullptr,
                           const char* fallbackPassword = nullptr);
  static bool saveMode(WifiMode mode);
  static bool saveStationCredentials(const char* ssid, const char* password);
  static bool clearStationCredentials();

  static const char* modeName(WifiMode mode);
  static String defaultApSsid();
  static constexpr const char* defaultApPassword() { return "cube-scrambler"; }

 private:
  static constexpr const char* kNamespace = "cube-net";
};

}  // namespace network
