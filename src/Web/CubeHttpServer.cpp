#include "CubeHttpServer.h"

#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WiFi.h>

#include "CubeHttpsServer.h"
#include "Move/MoveManager.h"
#include "Network/WifiSettings.h"

#if !defined(CUBE_DISTRIBUTION_BUILD) && __has_include("wifi_credentials.h")
#include "wifi_credentials.h"
#define CUBE_WIFI_CONFIGURED 1
#else
#define CUBE_WIFI_CONFIGURED 0
#endif

namespace web {
namespace {
constexpr uint16_t kHttpPort = 80;
constexpr uint32_t kWifiConnectTimeoutMs = 15000;
constexpr const char* kMdnsHost = "cube-scrambler";
constexpr const char* kWebPartitionLabel = "web";

String urlEncode(const String& value) {
  static constexpr char kHex[] = "0123456789ABCDEF";
  String encoded;
  encoded.reserve(value.length() * 3);
  for (size_t i = 0; i < value.length(); ++i) {
    const uint8_t ch = static_cast<uint8_t>(value[i]);
    const bool unreserved =
        (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
        (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' || ch == '.' ||
        ch == '~';
    if (unreserved) {
      encoded += static_cast<char>(ch);
    } else {
      encoded += '%';
      encoded += kHex[(ch >> 4) & 0x0F];
      encoded += kHex[ch & 0x0F];
    }
  }
  return encoded;
}
}  // namespace

CubeHttpServer::CubeHttpServer() : server_(kHttpPort) {}

bool CubeHttpServer::begin(solver::Min2PhaseSolver& solver,
                           move::MoveManager& moveManager,
                           sequence::SequenceGenerator& sequenceGenerator,
                           Stream& out) {
  out_ = &out;
  ready_ = false;
  httpsReady_ = false;
  accessPointMode_ = false;
  webUiReady_ = mountWebUi();

  if (moveManager.initializeRobot(out)) {
    out.println("CubeRobot standalone init: READY");
  } else {
    out.println("CubeRobot standalone init: FAILED");
  }
  out.flush();

#if CUBE_WIFI_CONFIGURED
  const network::WifiSettings wifiSettings =
      network::WifiSettingsStore::load(CUBE_WIFI_SSID, CUBE_WIFI_PASSWORD);
#else
  const network::WifiSettings wifiSettings = network::WifiSettingsStore::load();
#endif

  const auto startAccessPoint = [&]() -> bool {
    accessPointMode_ = true;
    WiFi.mode(WIFI_AP);
    const String apSsid = network::WifiSettingsStore::defaultApSsid();
    const char* apPassword = network::WifiSettingsStore::defaultApPassword();
    if (!WiFi.softAP(apSsid.c_str(), apPassword)) {
      out.println("Wi-Fi AP: FAILED_TO_START");
      out.flush();
      return false;
    }
    out.printf("Wi-Fi AP: READY ssid=%s ip=%s\n", apSsid.c_str(),
               WiFi.softAPIP().toString().c_str());
    out.printf("Wi-Fi AP password: %s\n", apPassword);
    return true;
  };

  if (wifiSettings.mode == network::WifiMode::AccessPoint) {
    if (!startAccessPoint()) return false;
  } else {
    bool stationConnected = false;

    if (wifiSettings.stationSsid.isEmpty()) {
      out.println("Wi-Fi station: no credentials available");
    } else {
      WiFi.mode(WIFI_STA);
      WiFi.begin(wifiSettings.stationSsid.c_str(),
                 wifiSettings.stationPassword.c_str());
      out.printf("Wi-Fi station: connecting to %s", wifiSettings.stationSsid.c_str());
      out.flush();

      const uint32_t started = millis();
      while (WiFi.status() != WL_CONNECTED &&
             millis() - started < kWifiConnectTimeoutMs) {
        delay(250);
        out.print('.');
        out.flush();
      }
      out.println();

      stationConnected = WiFi.status() == WL_CONNECTED;
      if (!stationConnected) {
        out.println("Wi-Fi station: FAILED_TO_CONNECT");
      }
    }

    if (!stationConnected) {
      out.println("Wi-Fi fallback: starting AP mode");
      WiFi.disconnect();
      delay(100);
      if (!startAccessPoint()) return false;
    }
  }

  if (MDNS.begin(kMdnsHost)) {
    MDNS.addService("http", "tcp", kHttpPort);
    out.printf("mDNS: READY %s.local\n", kMdnsHost);
  } else {
    out.println("mDNS: FAILED (IP access remains available)");
  }

  registerRoutes();
  server_.begin();
  ready_ = true;

  static CubeHttpsServer httpsServer;
  httpsReady_ =
      httpsServer.begin(solver, moveManager, sequenceGenerator, out);

  const String ip = localIp();
  if (accessPointMode_) {
    out.printf("Wi-Fi: AP mode ip=%s clients=%u\n", ip.c_str(),
               static_cast<unsigned>(WiFi.softAPgetStationNum()));
  } else {
    out.printf("Wi-Fi: CONNECTED ip=%s rssi=%d dBm\n", ip.c_str(), WiFi.RSSI());
  }
  if (httpsReady_) {
    out.printf("HTTP redirect: READY http://%s/ -> HTTPS\n", ip.c_str());
  } else {
    out.println("HTTP redirect: RECOVERY_ONLY (HTTPS unavailable)");
  }
  out.printf("Web UI: %s\n", webUiReady_ ? "READY" : "FALLBACK_ONLY");
  out.flush();
  return true;
}

bool CubeHttpServer::mountWebUi() {
  if (!out_) return false;

  if (!SPIFFS.begin(false, "/spiffs", 10, kWebPartitionLabel)) {
    out_->println("Web UI SPIFFS: NOT_MOUNTED");
    out_->println("Generate assets and flash the `web` filesystem partition.");
    out_->flush();
    return false;
  }

  if (!SPIFFS.exists("/index.html")) {
    out_->println("Web UI SPIFFS: MOUNTED but /index.html is missing");
    out_->flush();
    return false;
  }

  out_->printf("Web UI SPIFFS: READY used=%u total=%u bytes\n",
               static_cast<unsigned>(SPIFFS.usedBytes()),
               static_cast<unsigned>(SPIFFS.totalBytes()));
  out_->flush();
  return true;
}

void CubeHttpServer::registerRoutes() {
  server_.on("/ca.crt", HTTP_GET, [this]() { handleCaCertificate(); });
  server_.onNotFound([this]() { handleRedirect(); });
}

void CubeHttpServer::handleClient() {
  if (ready_) server_.handleClient();
}

String CubeHttpServer::localIp() const {
  if (!ready_) return String();
  return accessPointMode_ ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
}

String CubeHttpServer::redirectHost() const {
  String host = server_.hostHeader();
  if (host.endsWith(":80")) host.remove(host.length() - 3);
  if (host.isEmpty()) host = String(kMdnsHost) + ".local";
  return host;
}

void CubeHttpServer::handleRedirect() {
  if (!httpsReady_) {
    server_.sendHeader("Cache-Control", "no-store");
    server_.send(503, "text/plain; charset=utf-8",
                 "HTTPS application is unavailable. Check the NanoC6 serial log.\n");
    return;
  }

  String target = "https://" + redirectHost();
  String path = server_.uri();
  if (path.isEmpty()) path = "/";
  target += path;

  const int argCount = server_.args();
  if (argCount > 0) {
    target += '?';
    for (int i = 0; i < argCount; ++i) {
      if (i > 0) target += '&';
      target += urlEncode(server_.argName(i));
      target += '=';
      target += urlEncode(server_.arg(i));
    }
  }

  server_.sendHeader("Location", target, true);
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(307, "text/plain; charset=utf-8", "HTTPS required\n");
}

void CubeHttpServer::handleCaCertificate() {
  if (!webUiReady_ || !SPIFFS.exists("/ca.crt")) {
    server_.send(404, "text/plain; charset=utf-8", "CA certificate not found\n");
    return;
  }

  File file = SPIFFS.open("/ca.crt", FILE_READ);
  if (!file || file.isDirectory()) {
    if (file) file.close();
    server_.send(500, "text/plain; charset=utf-8", "Failed to open CA certificate\n");
    return;
  }

  server_.sendHeader("Cache-Control", "no-store");
  server_.streamFile(file, "application/x-x509-ca-cert");
  file.close();
}

}  // namespace web
