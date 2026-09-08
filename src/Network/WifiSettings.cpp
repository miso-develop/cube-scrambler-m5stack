#include "WifiSettings.h"

#include <Preferences.h>

namespace network {
namespace {
constexpr const char* kModeKey = "mode";
constexpr const char* kStationSsidKey = "sta_ssid";
constexpr const char* kStationPasswordKey = "sta_pass";
}

WifiSettings WifiSettingsStore::load(const char* fallbackSsid,
                                     const char* fallbackPassword) {
  WifiSettings settings{};
  Preferences preferences;
  if (!preferences.begin(kNamespace, true)) {
    if (fallbackSsid && fallbackSsid[0] != '\0') {
      settings.stationSsid = fallbackSsid;
      settings.stationPassword = fallbackPassword ? fallbackPassword : "";
      settings.mode = WifiMode::Station;
    } else {
      settings.mode = WifiMode::AccessPoint;
    }
    return settings;
  }

  const String storedMode =
      preferences.isKey(kModeKey) ? preferences.getString(kModeKey, "") : String();
  settings.stationSsid = preferences.isKey(kStationSsidKey)
                             ? preferences.getString(kStationSsidKey, "")
                             : String();
  settings.stationPassword = preferences.isKey(kStationPasswordKey)
                                 ? preferences.getString(kStationPasswordKey, "")
                                 : String();
  settings.stationCredentialsStored = settings.stationSsid.length() > 0;
  preferences.end();

  if (!settings.stationCredentialsStored && fallbackSsid && fallbackSsid[0] != '\0') {
    settings.stationSsid = fallbackSsid;
    settings.stationPassword = fallbackPassword ? fallbackPassword : "";
  }

  // The configured mode is independent of whether station credentials are
  // currently available. Runtime bootstrap decides whether a configured STA
  // mode must temporarily fall back to AP for recovery.
  if (storedMode == "ap") {
    settings.mode = WifiMode::AccessPoint;
  } else if (storedMode == "sta") {
    settings.mode = WifiMode::Station;
  } else {
    settings.mode = settings.stationSsid.length() > 0 ? WifiMode::Station
                                                      : WifiMode::AccessPoint;
  }

  return settings;
}

bool WifiSettingsStore::saveMode(WifiMode mode) {
  Preferences preferences;
  if (!preferences.begin(kNamespace, false)) return false;
  const size_t written =
      preferences.putString(kModeKey, mode == WifiMode::AccessPoint ? "ap" : "sta");
  preferences.end();
  return written > 0;
}

bool WifiSettingsStore::saveStationCredentials(const char* ssid,
                                               const char* password) {
  if (!ssid || ssid[0] == '\0') return false;
  const char* passwordValue = password ? password : "";

  Preferences preferences;
  if (!preferences.begin(kNamespace, false)) return false;
  const size_t ssidWritten = preferences.putString(kStationSsidKey, ssid);
  const size_t passwordWritten =
      preferences.putString(kStationPasswordKey, passwordValue);
  preferences.end();

  // Preferences::putString may report zero bytes for an empty string depending
  // on the backend implementation. Empty is a valid password for an open WLAN,
  // so only require a positive password write when a password was supplied.
  const bool passwordStored = passwordValue[0] == '\0' || passwordWritten > 0;
  return ssidWritten > 0 && passwordStored;
}

bool WifiSettingsStore::clearStationCredentials() {
  Preferences preferences;
  if (!preferences.begin(kNamespace, false)) return false;
  const bool ssidRemoved = preferences.remove(kStationSsidKey);
  const bool passwordRemoved = preferences.remove(kStationPasswordKey);
  preferences.end();
  return ssidRemoved || passwordRemoved;
}

const char* WifiSettingsStore::modeName(WifiMode mode) {
  return mode == WifiMode::AccessPoint ? "ap" : "sta";
}

String WifiSettingsStore::defaultApSsid() {
  const uint64_t mac = ESP.getEfuseMac();
  char suffix[7]{};
  snprintf(suffix, sizeof(suffix), "%06X",
           static_cast<unsigned>(mac & 0xFFFFFFULL));
  return String("CubeScrambler-") + suffix;
}

}  // namespace network
