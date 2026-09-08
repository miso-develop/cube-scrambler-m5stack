#include "CubeHttpsServer.h"

#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <cstdlib>
#include <esp_heap_caps.h>
#include <esp_https_server.h>

#include "Move/MoveManager.h"
#include "Move/MoveRunner.h"
#include "Move/MoveTypes.h"
#include "Network/WifiSettings.h"
#include "Sequence/SequenceGenerator.h"
#include "Solver/Min2PhaseSolver.h"

namespace web {
namespace {
constexpr const char* kServerCertPath = "/tls.crt";
constexpr const char* kPrivateKeyPath = "/tls.key";
constexpr const char* kSolvedFacelets =
    "UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB";
constexpr uint16_t kHttpsPort = 443;
constexpr size_t kQueryBufferSize = 512;
constexpr size_t kStaticChunkSize = 4096;
constexpr size_t kAcceptEncodingBufferSize = 128;

const char kFallbackIndexHtml[] = R"HTML(<!doctype html>
<html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Cube Scrambler NanoC6</title></head>
<body><h1>Cube Scrambler NanoC6</h1><p>HTTPS API is running, but Web UI assets are unavailable.</p></body></html>)HTML";

bool clientAcceptsGzip(httpd_req_t* req) {
  const size_t length = httpd_req_get_hdr_value_len(req, "Accept-Encoding");
  if (length == 0 || length >= kAcceptEncodingBufferSize) return false;
  char value[kAcceptEncodingBufferSize]{};
  if (httpd_req_get_hdr_value_str(req, "Accept-Encoding", value, sizeof(value)) != ESP_OK)
    return false;
  return String(value).indexOf("gzip") >= 0;
}

String jsonEscape(const String& value) {
  String escaped;
  escaped.reserve(value.length() + 8);
  for (size_t i = 0; i < value.length(); ++i) {
    const char ch = value[i];
    switch (ch) {
      case '\\': escaped += "\\\\"; break;
      case '"': escaped += "\\\""; break;
      case '\n': escaped += "\\n"; break;
      case '\r': escaped += "\\r"; break;
      case '\t': escaped += "\\t"; break;
      default:
        if (static_cast<uint8_t>(ch) >= 0x20) escaped += ch;
        break;
    }
  }
  return escaped;
}

bool parseIntStrict(const String& text, int& value) {
  if (text.isEmpty()) return false;
  char* end = nullptr;
  const long parsed = std::strtol(text.c_str(), &end, 10);
  if (!end || *end != '\0' || parsed < INT_MIN || parsed > INT_MAX) return false;
  value = static_cast<int>(parsed);
  return true;
}
}  // namespace

bool CubeHttpsServer::loadPemFile(const char* path, uint8_t*& buffer,
                                  size_t& length) {
  if (!SPIFFS.exists(path)) return false;
  File file = SPIFFS.open(path, FILE_READ);
  if (!file || file.isDirectory()) {
    if (file) file.close();
    return false;
  }

  const size_t fileSize = file.size();
  uint8_t* allocated = static_cast<uint8_t*>(malloc(fileSize + 1));
  if (!allocated) {
    file.close();
    return false;
  }
  const size_t read = file.read(allocated, fileSize);
  file.close();
  if (read != fileSize) {
    free(allocated);
    return false;
  }
  allocated[fileSize] = '\0';
  buffer = allocated;
  length = fileSize + 1;
  return true;
}

bool CubeHttpsServer::begin(solver::Min2PhaseSolver& solver,
                            move::MoveManager& moveManager,
                            sequence::SequenceGenerator& sequenceGenerator,
                            Stream& out) {
  solver_ = &solver;
  moveManager_ = &moveManager;
  sequenceGenerator_ = &sequenceGenerator;
  out_ = &out;
  ready_ = false;

  if (!SPIFFS.exists(kServerCertPath) || !SPIFFS.exists(kPrivateKeyPath)) {
    out.println("HTTPS server: DISABLED (tls.crt/tls.key not found in Web SPIFFS)");
    out.flush();
    return false;
  }
  if (!loadPemFile(kServerCertPath, serverCert_, serverCertLen_) ||
      !loadPemFile(kPrivateKeyPath, privateKey_, privateKeyLen_)) {
    out.println("HTTPS server: FAILED_TO_LOAD_CERTIFICATE");
    out.flush();
    return false;
  }

  heapBeforeStart_ = ESP.getFreeHeap();
  httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
  config.port_secure = kHttpsPort;
  config.servercert = serverCert_;
  config.servercert_len = serverCertLen_;
  config.prvtkey_pem = privateKey_;
  config.prvtkey_len = privateKeyLen_;
  config.httpd.max_open_sockets = 3;
  config.httpd.max_uri_handlers = 12;
  config.httpd.lru_purge_enable = true;
  config.httpd.stack_size = 12288;
  config.httpd.uri_match_fn = httpd_uri_match_wildcard;

  const esp_err_t result = httpd_ssl_start(&server_, &config);
  if (result != ESP_OK || server_ == nullptr) {
    out.printf("HTTPS server: START_FAILED err=%d\n", static_cast<int>(result));
    out.flush();
    return false;
  }

  struct Route {
    const char* uri;
    httpd_method_t method;
    esp_err_t (*handler)(httpd_req_t*);
  };
  const Route routes[] = {
      {"/", HTTP_GET, &CubeHttpsServer::handleRoot},
      {"/api/status", HTTP_GET, &CubeHttpsServer::handleStatus},
      {"/api/https-status", HTTP_GET, &CubeHttpsServer::handleHttpsStatus},
      {"/api/settings", HTTP_GET, &CubeHttpsServer::handleSettingsGet},
      {"/api/settings", HTTP_POST, &CubeHttpsServer::handleSettingsPost},
      {"/api/solve", HTTP_GET, &CubeHttpsServer::handleSolve},
      {"/api/scramble", HTTP_GET, &CubeHttpsServer::handleScramble},
      {"/api/step", HTTP_GET, &CubeHttpsServer::handleStep},
      {"/api/sequence", HTTP_GET, &CubeHttpsServer::handleSequence},
      {"/api/sequence", HTTP_POST, &CubeHttpsServer::handleSequence},
      {"/api/stop", HTTP_POST, &CubeHttpsServer::handleStop},
      {"/*", HTTP_GET, &CubeHttpsServer::handleStatic},
  };

  for (const auto& route : routes) {
    httpd_uri_t uri{};
    uri.uri = route.uri;
    uri.method = route.method;
    uri.handler = route.handler;
    uri.user_ctx = this;
    if (httpd_register_uri_handler(server_, &uri) != ESP_OK) {
      out.printf("HTTPS server: URI_REGISTER_FAILED %s\n", route.uri);
      httpd_ssl_stop(server_);
      server_ = nullptr;
      out.flush();
      return false;
    }
  }

  heapAfterStart_ = ESP.getFreeHeap();
  ready_ = true;
  MDNS.addService("https", "tcp", kHttpsPort);
  out.printf("HTTPS server: READY https://cube-scrambler.local/ heap=%u -> %u\n",
             static_cast<unsigned>(heapBeforeStart_),
             static_cast<unsigned>(heapAfterStart_));
  out.flush();
  return true;
}

const char* CubeHttpsServer::httpStatus(int statusCode) {
  switch (statusCode) {
    case 200: return "200 OK";
    case 202: return "202 Accepted";
    case 400: return "400 Bad Request";
    case 404: return "404 Not Found";
    case 409: return "409 Conflict";
    case 500: return "500 Internal Server Error";
    case 503: return "503 Service Unavailable";
    default: return "500 Internal Server Error";
  }
}

esp_err_t CubeHttpsServer::sendJson(httpd_req_t* req, int statusCode,
                                    const char* body) {
  httpd_resp_set_status(req, httpStatus(statusCode));
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, body, HTTPD_RESP_USE_STRLEN);
}

bool CubeHttpsServer::queryArg(httpd_req_t* req, const char* key,
                               String& value) const {
  const size_t queryLength = httpd_req_get_url_query_len(req);
  if (queryLength == 0 || queryLength >= kQueryBufferSize) return false;
  char query[kQueryBufferSize];
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) return false;
  char decoded[kQueryBufferSize];
  if (httpd_query_key_value(query, key, decoded, sizeof(decoded)) != ESP_OK) return false;
  value = decoded;
  return true;
}

const char* CubeHttpsServer::contentTypeFor(const String& path) const {
  if (path.endsWith(".html")) return "text/html; charset=utf-8";
  if (path.endsWith(".css")) return "text/css; charset=utf-8";
  if (path.endsWith(".js")) return "application/javascript; charset=utf-8";
  if (path.endsWith(".json")) return "application/json; charset=utf-8";
  if (path.endsWith(".png")) return "image/png";
  if (path.endsWith(".ico")) return "image/x-icon";
  if (path.endsWith(".svg")) return "image/svg+xml";
  if (path.endsWith(".crt")) return "application/x-x509-ca-cert";
  return "application/octet-stream";
}

esp_err_t CubeHttpsServer::serveStatic(httpd_req_t* req,
                                       const String& requestPath) const {
  if (requestPath.indexOf("..") >= 0) return ESP_FAIL;
  String path = requestPath;
  const int queryIndex = path.indexOf('?');
  if (queryIndex >= 0) path.remove(queryIndex);
  if (path.isEmpty() || path == "/") path = "/index.html";
  if (path.endsWith("/")) path += "index.html";
  if (path == "/tls.key" || path == "/tls.crt") return ESP_FAIL;
  if (!SPIFFS.exists(path)) return ESP_FAIL;

  String filePath = path;
  bool gzip = false;
  const String gzipPath = path + ".gz";
  const bool gzipAvailable = SPIFFS.exists(gzipPath);
  if (gzipAvailable && clientAcceptsGzip(req)) {
    filePath = gzipPath;
    gzip = true;
  }

  File file = SPIFFS.open(filePath, FILE_READ);
  if (!file || file.isDirectory()) {
    if (file) file.close();
    return ESP_FAIL;
  }

  const size_t fileSize = file.size();
  const uint32_t started = millis();
  size_t totalSent = 0;

  httpd_resp_set_type(req, contentTypeFor(path));
  httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=3600");
  if (gzipAvailable) httpd_resp_set_hdr(req, "Vary", "Accept-Encoding");
  if (gzip) httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  if (path.endsWith(".html")) {
    httpd_resp_set_hdr(req, "Permissions-Policy", "camera=(self)");
  }

  char buffer[kStaticChunkSize];
  while (file.available()) {
    const size_t read = file.readBytes(buffer, sizeof(buffer));
    if (read == 0) break;
    if (httpd_resp_send_chunk(req, buffer, read) != ESP_OK) {
      file.close();
      httpd_resp_send_chunk(req, nullptr, 0);
      if (out_) {
        out_->printf(
            "Web static: path=%s encoding=%s bytes=%u/%u elapsed=%u ms result=FAIL\n",
            path.c_str(), gzip ? "gzip" : "identity",
            static_cast<unsigned>(totalSent), static_cast<unsigned>(fileSize),
            static_cast<unsigned>(millis() - started));
        out_->flush();
      }
      return ESP_FAIL;
    }
    totalSent += read;
  }
  file.close();

  const esp_err_t result = httpd_resp_send_chunk(req, nullptr, 0);
  if (out_) {
    out_->printf(
        "Web static: path=%s encoding=%s bytes=%u/%u elapsed=%u ms result=%s\n",
        path.c_str(), gzip ? "gzip" : "identity",
        static_cast<unsigned>(totalSent), static_cast<unsigned>(fileSize),
        static_cast<unsigned>(millis() - started), result == ESP_OK ? "OK" : "FAIL");
    out_->flush();
  }
  return result;
}

esp_err_t CubeHttpsServer::handleRoot(httpd_req_t* req) {
  auto* self = static_cast<CubeHttpsServer*>(req->user_ctx);
  if (self && self->serveStatic(req, "/index.html") == ESP_OK) return ESP_OK;
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  return httpd_resp_send(req, kFallbackIndexHtml, HTTPD_RESP_USE_STRLEN);
}

esp_err_t CubeHttpsServer::handleStatic(httpd_req_t* req) {
  auto* self = static_cast<CubeHttpsServer*>(req->user_ctx);
  const String path = req->uri ? String(req->uri) : String();
  if (path.startsWith("/api/")) {
    return sendJson(req, 404, "{\"error\":\"not_found\"}");
  }
  if (self && self->serveStatic(req, path) == ESP_OK) return ESP_OK;
  httpd_resp_set_status(req, "404 Not Found");
  httpd_resp_set_type(req, "text/plain; charset=utf-8");
  return httpd_resp_sendstr(req, "Not found");
}

esp_err_t CubeHttpsServer::handleStatus(httpd_req_t* req) {
  auto* self = static_cast<CubeHttpsServer*>(req->user_ctx);
  move::MoveRunner::Status moveStatus{};
  const bool hasMoveManager = self && self->moveManager_;
  if (hasMoveManager) moveStatus = self->moveManager_->runnerStatus();

  char json[416];
  snprintf(json, sizeof(json),
           "{\"status\":\"%s\",\"solverReady\":%s,\"robotReady\":%s,"
           "\"webUiReady\":%s,\"httpsReady\":%s,\"moveIndex\":%u,"
           "\"moveCount\":%u,\"heap\":%u,\"rssi\":%d}",
           hasMoveManager ? move::MoveRunner::stateName(moveStatus.state) : "idle",
           self && self->solver_ && self->solver_->ready() ? "true" : "false",
           hasMoveManager && self->moveManager_->robotReady() ? "true" : "false",
           SPIFFS.exists("/index.html") ? "true" : "false",
           self && self->ready_ ? "true" : "false",
           static_cast<unsigned>(moveStatus.currentMove),
           static_cast<unsigned>(moveStatus.totalMoves),
           static_cast<unsigned>(ESP.getFreeHeap()), WiFi.RSSI());
  return sendJson(req, 200, json);
}

esp_err_t CubeHttpsServer::handleHttpsStatus(httpd_req_t* req) {
  auto* self = static_cast<CubeHttpsServer*>(req->user_ctx);
  char json[192];
  snprintf(json, sizeof(json),
           "{\"httpsReady\":%s,\"heap\":%u,\"minimumHeap\":%u}",
           self && self->ready_ ? "true" : "false",
           static_cast<unsigned>(ESP.getFreeHeap()),
           static_cast<unsigned>(heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT)));
  return sendJson(req, 200, json);
}

esp_err_t CubeHttpsServer::handleSettingsGet(httpd_req_t* req) {
  auto* self = static_cast<CubeHttpsServer*>(req->user_ctx);
  if (!self || !self->moveManager_)
    return sendJson(req, 503, "{\"error\":\"settings_not_ready\"}");

  const network::WifiSettings wifi = network::WifiSettingsStore::load();
  const bool activeAp = WiFi.getMode() == WIFI_AP;
  const String activeIp = activeAp ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  const auto& robot = self->moveManager_->robot();

  String json;
  json.reserve(640);
  json += "{\"wifi\":{";
  json += "\"configuredMode\":\"";
  json += network::WifiSettingsStore::modeName(wifi.mode);
  json += "\",\"activeMode\":\"";
  json += activeAp ? "ap" : "sta";
  json += "\",\"activeIp\":\"" + jsonEscape(activeIp) + "\"";
  json += ",\"stationCredentialsStored\":";
  json += wifi.stationCredentialsStored ? "true" : "false";
  json += ",\"stationSsid\":\"" + jsonEscape(wifi.stationSsid) + "\"";
  json += ",\"defaultApSsid\":\"" +
          jsonEscape(network::WifiSettingsStore::defaultApSsid()) + "\"},";
  json += "\"servo\":{";
  json += "\"stand-correct\":" + String(robot.standCorrectionAngle());
  json += ",\"stand-turn\":" + String(robot.standTurnAngle());
  json += ",\"arm-pull\":" + String(robot.armPullAngle());
  json += ",\"arm-hold\":" + String(robot.armHoldAngle());
  json += ",\"arm-release\":" + String(robot.armReleaseAngle());
  json += ",\"arm-ready\":" + String(robot.armReadyAngle());
  json += ",\"servo-sleep\":" + String(robot.servoSleepMs());
  json += ",\"arm-x-sleep\":" + String(robot.armXSleepMs());
  json += "}}";
  return sendJson(req, 200, json.c_str());
}

esp_err_t CubeHttpsServer::handleSettingsPost(httpd_req_t* req) {
  auto* self = static_cast<CubeHttpsServer*>(req->user_ctx);
  if (!self || !self->moveManager_ || !self->out_)
    return sendJson(req, 503, "{\"error\":\"settings_not_ready\"}");

  String action;
  if (!self->queryArg(req, "action", action))
    return sendJson(req, 400, "{\"error\":\"action_required\"}");

  if (action == "wifi-mode") {
    String mode;
    if (!self->queryArg(req, "mode", mode))
      return sendJson(req, 400, "{\"error\":\"mode_required\"}");
    network::WifiMode target;
    if (mode == "sta") target = network::WifiMode::Station;
    else if (mode == "ap") target = network::WifiMode::AccessPoint;
    else return sendJson(req, 400, "{\"error\":\"invalid_mode\"}");
    if (!network::WifiSettingsStore::saveMode(target))
      return sendJson(req, 500, "{\"error\":\"wifi_mode_save_failed\"}");
    return sendJson(req, 200, "{\"saved\":true,\"rebootRequired\":true}");
  }

  if (action == "wifi-set") {
    String ssid;
    String password;
    if (!self->queryArg(req, "ssid", ssid) || !self->queryArg(req, "password", password))
      return sendJson(req, 400, "{\"error\":\"ssid_password_required\"}");
    ssid.trim();
    if (ssid.isEmpty() || ssid.length() > 32)
      return sendJson(req, 400, "{\"error\":\"ssid_must_be_1_to_32_chars\"}");
    if (!password.isEmpty() && (password.length() < 8 || password.length() > 63))
      return sendJson(req, 400, "{\"error\":\"password_must_be_empty_or_8_to_63_chars\"}");
    if (!network::WifiSettingsStore::saveStationCredentials(ssid.c_str(), password.c_str()))
      return sendJson(req, 500, "{\"error\":\"wifi_credentials_save_failed\"}");
    return sendJson(req, 200, "{\"saved\":true,\"rebootRequired\":true}");
  }

  if (action == "wifi-clear") {
    const bool changed = network::WifiSettingsStore::clearStationCredentials();
    return sendJson(req, 200,
                    changed ? "{\"saved\":true,\"rebootRequired\":true}"
                            : "{\"saved\":false,\"rebootRequired\":true}");
  }

  if (action == "servo-set") {
    if (self->moveManager_->isBusy())
      return sendJson(req, 409, "{\"error\":\"busy\"}");
    String key;
    String valueText;
    int value = 0;
    if (!self->queryArg(req, "key", key) || !self->queryArg(req, "value", valueText) ||
        !parseIntStrict(valueText, value)) {
      return sendJson(req, 400, "{\"error\":\"invalid_servo_setting\"}");
    }
    if (!self->moveManager_->robot().setCalibration(key.c_str(), value, *self->out_))
      return sendJson(req, 400, "{\"error\":\"servo_setting_rejected\"}");
    return sendJson(req, 200, "{\"saved\":true,\"effectiveImmediately\":true}");
  }

  if (action == "reboot") {
    if (self->moveManager_->isBusy())
      return sendJson(req, 409, "{\"error\":\"busy\"}");
    const esp_err_t result = sendJson(req, 202, "{\"accepted\":true,\"rebooting\":true}");
    self->out_->println("Reboot requested from Web settings");
    self->out_->flush();
    delay(250);
    ESP.restart();
    return result;
  }

  return sendJson(req, 400, "{\"error\":\"unknown_settings_action\"}");
}

esp_err_t CubeHttpsServer::handleSolve(httpd_req_t* req) {
  auto* self = static_cast<CubeHttpsServer*>(req->user_ctx);
  if (!self || !self->solver_ || !self->solver_->ready())
    return sendJson(req, 503, "{\"error\":\"solver_not_ready\"}");
  if (!self->moveManager_ || !self->out_)
    return sendJson(req, 503, "{\"error\":\"move_manager_not_ready\"}");
  if (!self->moveManager_->robotReady())
    return sendJson(req, 409, "{\"error\":\"robot_not_ready\"}");
  if (!self->moveManager_->canStart())
    return sendJson(req, 409, "{\"error\":\"busy\"}");

  String facelets;
  if (!self->queryArg(req, "facelets", facelets))
    return sendJson(req, 400, "{\"error\":\"facelets_required\"}");
  if (facelets.length() != 54)
    return sendJson(req, 400, "{\"error\":\"invalid_facelets_length\"}");
  if (facelets == kSolvedFacelets)
    return sendJson(req, 400, "{\"error\":\"cube_initial_state\"}");

  const solver::Min2PhaseSolveResult result = self->solver_->solve(facelets.c_str(), 21);
  if (!result.success) {
    char json[96];
    snprintf(json, sizeof(json),
             "{\"error\":\"solve_failed\",\"code\":%d,\"elapsedMs\":%u}",
             result.error, static_cast<unsigned>(result.elapsedMs));
    return sendJson(req, 400, json);
  }

  move::RobotMoveSequence converted{};
  if (!self->moveManager_->convertText(result.moves, converted, self->out_))
    return sendJson(req, 500, "{\"error\":\"solve_conversion_failed\"}");
  if (!self->moveManager_->startConverted(converted, *self->out_))
    return sendJson(req, 503, "{\"error\":\"sequence_start_failed\"}");

  char json[512];
  snprintf(json, sizeof(json),
           "{\"accepted\":true,\"status\":\"running\",\"facelets\":\"%s\","
           "\"sequence\":\"%s\",\"robotMoves\":%u,\"elapsedMs\":%u,\"length\":%u}",
           facelets.c_str(), result.moves, static_cast<unsigned>(converted.count),
           static_cast<unsigned>(result.elapsedMs), static_cast<unsigned>(result.length));
  return sendJson(req, 202, json);
}

esp_err_t CubeHttpsServer::handleSequence(httpd_req_t* req) {
  auto* self = static_cast<CubeHttpsServer*>(req->user_ctx);
  if (!self || !self->moveManager_)
    return sendJson(req, 503, "{\"error\":\"move_manager_not_ready\"}");
  if (!self->moveManager_->robotReady())
    return sendJson(req, 409, "{\"error\":\"robot_not_ready\"}");
  if (!self->moveManager_->canStart())
    return sendJson(req, 409, "{\"error\":\"busy\"}");

  String sequence;
  if (!self->queryArg(req, "sequence", sequence))
    return sendJson(req, 400, "{\"error\":\"sequence_required\"}");
  move::RobotMoveSequence converted{};
  if (!self->moveManager_->convertText(sequence.c_str(), converted, self->out_))
    return sendJson(req, 400, "{\"error\":\"invalid_sequence\"}");
  if (!self->out_ || !self->moveManager_->startConverted(converted, *self->out_))
    return sendJson(req, 503, "{\"error\":\"sequence_start_failed\"}");

  char json[128];
  snprintf(json, sizeof(json),
           "{\"accepted\":true,\"status\":\"running\",\"robotMoves\":%u}",
           static_cast<unsigned>(converted.count));
  return sendJson(req, 202, json);
}

esp_err_t CubeHttpsServer::handleScramble(httpd_req_t* req) {
  auto* self = static_cast<CubeHttpsServer*>(req->user_ctx);
  if (!self || !self->sequenceGenerator_ || !self->moveManager_ || !self->out_)
    return sendJson(req, 503, "{\"error\":\"sequence_generator_not_ready\"}");
  if (!self->moveManager_->robotReady())
    return sendJson(req, 409, "{\"error\":\"robot_not_ready\"}");
  if (!self->moveManager_->canStart())
    return sendJson(req, 409, "{\"error\":\"busy\"}");

  String type;
  if (self->queryArg(req, "type", type) && type != "0")
    return sendJson(req, 400, "{\"error\":\"unsupported_scramble_type\"}");

  sequence::GeneratedSequenceResult generated{};
  if (!self->sequenceGenerator_->random(generated, self->out_)) {
    char json[96];
    snprintf(json, sizeof(json),
             "{\"error\":\"scramble_generation_failed\",\"code\":%d}", generated.error);
    return sendJson(req, 500, json);
  }

  move::RobotMoveSequence converted{};
  if (!self->moveManager_->convertText(generated.sequence, converted, self->out_))
    return sendJson(req, 500, "{\"error\":\"scramble_conversion_failed\"}");
  if (!self->moveManager_->startConverted(converted, *self->out_))
    return sendJson(req, 503, "{\"error\":\"scramble_start_failed\"}");

  char json[512];
  snprintf(json, sizeof(json),
           "{\"accepted\":true,\"status\":\"running\",\"facelets\":\"%s\","
           "\"sequence\":\"%s\",\"robotMoves\":%u,\"generationMs\":%u}",
           generated.facelets, generated.sequence, static_cast<unsigned>(converted.count),
           static_cast<unsigned>(generated.generationMs));
  return sendJson(req, 202, json);
}

esp_err_t CubeHttpsServer::handleStep(httpd_req_t* req) {
  auto* self = static_cast<CubeHttpsServer*>(req->user_ctx);
  if (!self || !self->sequenceGenerator_ || !self->moveManager_ || !self->out_)
    return sendJson(req, 503, "{\"error\":\"sequence_generator_not_ready\"}");
  if (!self->moveManager_->robotReady())
    return sendJson(req, 409, "{\"error\":\"robot_not_ready\"}");
  if (!self->moveManager_->canStart())
    return sendJson(req, 409, "{\"error\":\"busy\"}");

  String number;
  if (!self->queryArg(req, "number", number))
    return sendJson(req, 400, "{\"error\":\"step_number_required\"}");
  const int stepNumber = number.toInt();
  if (stepNumber < 2 || stepNumber > 7)
    return sendJson(req, 400, "{\"error\":\"invalid_step_number\"}");

  sequence::GeneratedSequenceResult generated{};
  if (!self->sequenceGenerator_->step(static_cast<uint8_t>(stepNumber), generated,
                                      self->out_)) {
    char json[96];
    snprintf(json, sizeof(json),
             "{\"error\":\"step_generation_failed\",\"code\":%d}", generated.error);
    return sendJson(req, 500, json);
  }

  move::RobotMoveSequence converted{};
  if (!self->moveManager_->convertText(generated.sequence, converted, self->out_))
    return sendJson(req, 500, "{\"error\":\"step_conversion_failed\"}");
  if (!self->moveManager_->startConverted(converted, *self->out_))
    return sendJson(req, 503, "{\"error\":\"sequence_start_failed\"}");

  char json[512];
  snprintf(json, sizeof(json),
           "{\"accepted\":true,\"status\":\"running\",\"step\":%d,"
           "\"facelets\":\"%s\",\"sequence\":\"%s\",\"robotMoves\":%u,"
           "\"generationMs\":%u}",
           stepNumber, generated.facelets, generated.sequence,
           static_cast<unsigned>(converted.count),
           static_cast<unsigned>(generated.generationMs));
  return sendJson(req, 202, json);
}

esp_err_t CubeHttpsServer::handleStop(httpd_req_t* req) {
  auto* self = static_cast<CubeHttpsServer*>(req->user_ctx);
  if (!self || !self->moveManager_)
    return sendJson(req, 503, "{\"error\":\"move_manager_not_ready\"}");
  if (!self->moveManager_->isBusy())
    return sendJson(req, 409, "{\"error\":\"not_running\"}");
  self->moveManager_->requestStop();
  return sendJson(req, 202, "{\"accepted\":true,\"status\":\"stopping\"}");
}

}  // namespace web
