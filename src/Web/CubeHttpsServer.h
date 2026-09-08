#pragma once

#include <Arduino.h>
#include <esp_http_server.h>

namespace solver {
class Min2PhaseSolver;
}

namespace move {
class MoveManager;
}

namespace sequence {
class SequenceGenerator;
}

namespace web {
namespace detail {

// ESP-IDF's httpd_query_key_value() extracts but does not URI-decode values.
// Existing Cube UI requests use URLSearchParams, so move notation arrives as
// e.g. R%20U%20R%27. Decode in place before the application parser sees it.
inline int hexNibble(char ch) {
  if (ch >= '0' && ch <= '9') return ch - '0';
  if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
  if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
  return -1;
}

inline esp_err_t queryKeyValueDecoded(const char* query, const char* key,
                                      char* value, size_t valueSize) {
  const esp_err_t result = ::httpd_query_key_value(query, key, value, valueSize);
  if (result != ESP_OK) return result;

  size_t read = 0;
  size_t write = 0;
  while (value[read] != '\0') {
    if (value[read] == '%' && value[read + 1] != '\0' &&
        value[read + 2] != '\0') {
      const int high = hexNibble(value[read + 1]);
      const int low = hexNibble(value[read + 2]);
      if (high >= 0 && low >= 0) {
        value[write++] = static_cast<char>((high << 4) | low);
        read += 3;
        continue;
      }
    }
    value[write++] = value[read] == '+' ? ' ' : value[read];
    ++read;
  }
  value[write] = '\0';
  return ESP_OK;
}

}  // namespace detail
}  // namespace web

#define httpd_query_key_value web::detail::queryKeyValueDecoded

namespace web {

class CubeHttpsServer {
 public:
  bool begin(solver::Min2PhaseSolver& solver, move::MoveManager& moveManager,
             sequence::SequenceGenerator& sequenceGenerator, Stream& out);

  bool ready() const { return ready_; }
  uint32_t heapBeforeStart() const { return heapBeforeStart_; }
  uint32_t heapAfterStart() const { return heapAfterStart_; }

 private:
  static esp_err_t handleRoot(httpd_req_t* req);
  static esp_err_t handleStatus(httpd_req_t* req);
  static esp_err_t handleHttpsStatus(httpd_req_t* req);
  static esp_err_t handleSettingsGet(httpd_req_t* req);
  static esp_err_t handleSettingsPost(httpd_req_t* req);
  static esp_err_t handleSolve(httpd_req_t* req);
  static esp_err_t handleSequence(httpd_req_t* req);
  static esp_err_t handleScramble(httpd_req_t* req);
  static esp_err_t handleStep(httpd_req_t* req);
  static esp_err_t handleStop(httpd_req_t* req);
  static esp_err_t handleStatic(httpd_req_t* req);

  bool loadPemFile(const char* path, uint8_t*& buffer, size_t& length);
  bool queryArg(httpd_req_t* req, const char* key, String& value) const;
  esp_err_t serveStatic(httpd_req_t* req, const String& requestPath) const;
  const char* contentTypeFor(const String& path) const;
  static esp_err_t sendJson(httpd_req_t* req, int statusCode,
                            const char* body);
  static const char* httpStatus(int statusCode);

  httpd_handle_t server_ = nullptr;
  solver::Min2PhaseSolver* solver_ = nullptr;
  move::MoveManager* moveManager_ = nullptr;
  sequence::SequenceGenerator* sequenceGenerator_ = nullptr;
  Stream* out_ = nullptr;
  uint8_t* serverCert_ = nullptr;
  size_t serverCertLen_ = 0;
  uint8_t* privateKey_ = nullptr;
  size_t privateKeyLen_ = 0;
  uint32_t heapBeforeStart_ = 0;
  uint32_t heapAfterStart_ = 0;
  bool ready_ = false;
};

}  // namespace web
