#pragma once

#include <Arduino.h>
#include <WebServer.h>

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

class CubeHttpServer {
 public:
  CubeHttpServer();

  bool begin(solver::Min2PhaseSolver& solver, move::MoveManager& moveManager,
             sequence::SequenceGenerator& sequenceGenerator, Stream& out);
  void handleClient();

  bool ready() const { return ready_; }
  bool webUiReady() const { return webUiReady_; }
  bool httpsReady() const { return httpsReady_; }
  bool accessPointMode() const { return accessPointMode_; }
  String localIp() const;

 private:
  void registerRoutes();
  void handleRedirect();
  void handleCaCertificate();
  bool mountWebUi();
  String redirectHost() const;

  WebServer server_;
  Stream* out_ = nullptr;
  bool ready_ = false;
  bool webUiReady_ = false;
  bool httpsReady_ = false;
  bool accessPointMode_ = false;
};

}  // namespace web
