#include <Arduino.h>
#include <cstdio>
#include <cstring>
#include <limits>
#include "esp_chip_info.h"
#include "esp_heap_caps.h"

#include "Hardware/HardwareControls.h"
#include "Move/MoveManager.h"
#include "Network/WifiSettings.h"
#include "Robot/CubeRobot.h"
#include "Sequence/SequenceGenerator.h"
#include "Solver/Min2PhaseSolver.h"
#include "Solver/SolverProbe.h"
#include "Solver/SolverTableCatalog.h"
#include "Solver/SolverTableStorage.h"
#include "Web/CubeHttpServer.h"

namespace {
constexpr uint32_t kSerialBaud = 115200;
constexpr size_t kSerialCommandBufferSize = 256;

solver::SolverTableStorage solverTableStorage;
solver::Min2PhaseSolver min2phaseSolver;
web::CubeHttpServer cubeHttpServer;
robot::CubeRobot cubeRobot;
move::MoveManager moveManager(cubeRobot);
hardware::HardwareControls hardwareControls(moveManager);
sequence::SequenceGenerator sequenceGenerator(min2phaseSolver);
bool realTablesReady = false;
char serialCommandBuffer[kSerialCommandBufferSize]{};
size_t serialCommandLength = 0;

struct SolverBenchmarkCase {
  const char* scramble;
  const char* facelets;
};

constexpr SolverBenchmarkCase kSolverBenchmarkCases[] = {
    {"B U' L F U' R' B U F U' F' L B' D F' L2 D F2 U B",
     "DRLFULFDURFDLRRDUULFFBFDDBBFRRLDRUBRBUULLUBURFDLFBBBDL"},
    {"F2 D B' U2 R' D' L' F' U F2 U R2 D2 L D2 F' D' R' F' R",
     "DLUUUFFBBRLBBRLBBDURUUFUUFRFDDDDDDRFFRRRLFLLLLURBBFLDB"},
    {"D2 L' F' D F D2 L F' D B2 L D F' D' B' L2 F L U F'",
     "DFLUUBUDRFDDFRDLFDBLURFULLUDUBBDLUDBFBRRLULLBFRRFBBRRF"},
    {"R' B' U2 B' L' D' R' U' L' D L2 F2 U' L' F' U' L' F D R'",
     "DBBRUUBFUFBRDRLFURRDLDFBLULBFDUDLBFUFDUBLLDRUDRRFBLFRL"},
    {"F2 U L2 F R' U B R B' R2 B R' D2 L2 B R U2 R' F' R",
     "DBRBUFRRFDRBFRLLLLBDLUFDBLULDBLDFFUFFDDULBRBDURRUBRUFU"},
};

void printSystemInfo() {
  esp_chip_info_t chipInfo{};
  esp_chip_info(&chipInfo);

  Serial.println();
  Serial.printf("Cube Scrambler %s\n", hardware::HardwareControls::deviceName());
  Serial.printf("Chip model: %s\n", ESP.getChipModel());
  Serial.printf("CPU frequency: %u MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("Flash size: %u bytes\n", ESP.getFlashChipSize());
  Serial.printf("Cores: %d\n", chipInfo.cores);
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.println("READY");
  Serial.flush();
}

bool printTableImageStatus() {
  if (!solverTableStorage.validateImage(Serial)) return false;

  solver::SolverTableSectionView probeSection;
  if (solverTableStorage.findSection("probe_payload", probeSection)) {
    Serial.println("Solver table image contains probe payload, not real tables");
    return false;
  }

  return solver::validateMin2PhaseTableCatalog(solverTableStorage, Serial);
}

bool ensureSolverReady() {
  if (!realTablesReady) {
    Serial.println("Solver integration: TABLES_NOT_READY");
    Serial.flush();
    return false;
  }
  if (min2phaseSolver.ready()) return true;
  if (!min2phaseSolver.begin(solverTableStorage, Serial)) {
    Serial.println("Solver integration: FAILED_TO_BIND");
    Serial.flush();
    return false;
  }
  return true;
}

void printHelp() {
  Serial.println();
  Serial.println("Diagnostic commands:");
  Serial.println("  solver-test             Run the Min2Phase self-test");
  Serial.println("  solver-bench            Run 5 fixed 20-move solve benchmarks");
  Serial.println("  solver-status           Show solver/table readiness");
  Serial.println("  sequence-self-test      Generate random + step 2..7 targets (no servo)");
  Serial.println("  web-status              Show active Wi-Fi/HTTPS readiness");
  Serial.println("  wifi-status             Show persistent Wi-Fi settings (password hidden)");
  Serial.println("  wifi-mode sta|ap        Persist Wi-Fi mode; reboot required");
  Serial.println("  wifi-set <ssid>|<pass>  Persist station credentials; use empty pass for open Wi-Fi");
  Serial.println("  wifi-clear              Remove stored station credentials");
  Serial.println("  reboot                  Restart device when runner is idle");
  Serial.println("  move-self-test          Test parser/converter without servo movement");
  Serial.println("  move-convert <sequence> Convert Cube notation to robot moves");
  Serial.println("  move-start <sequence>   Start async sequence worker");
  Serial.println("  move-status             Show async sequence state/progress");
  Serial.println("  move-stop               Request async sequence stop");
  Serial.println("  move-run <sequence>     Blocking diagnostic fallback");
  Serial.println("  run <sequence>          Alias of move-run");
  Serial.println("  servo-status            Show direct-servo state");
  Serial.println("  servo-config            Show runtime servo calibration");
  Serial.println("  servo-set <key> <deg>   Set runtime calibration while runner is idle");
  Serial.println("    keys: stand-correct, stand-turn, arm-pull, arm-hold, arm-release, arm-ready");
  Serial.println("  servo-init              Attach PWM and move to Stand init + Arm ready");
  Serial.println("  servo-off               Detach both servo PWM outputs");
  Serial.println("  robot-d / robot-d2      Execute D / D2");
  Serial.println("  robot-dp / robot-dp2    Execute D' / D'2");
  Serial.println("  robot-x / x2 / x3       Execute x variants (robot-x2 etc.)");
  Serial.println("  robot-y / y2            Execute y variants");
  Serial.println("  robot-yp / yp2          Execute y' variants");
  Serial.println("  help                    Show this help");
  Serial.flush();
}

void printSolverStatus() {
  Serial.println();
  Serial.println("Solver status");
  Serial.printf("Table storage mapped: %s\n",
                solverTableStorage.isMapped() ? "YES" : "NO");
  Serial.printf("Real table catalog: %s\n", realTablesReady ? "READY" : "NOT_READY");
  Serial.printf("Solver core ready: %s\n", min2phaseSolver.ready() ? "YES" : "NO");
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.flush();
}

void printWebStatus() {
  Serial.println();
  Serial.println("Web status");
  Serial.printf("Web bootstrap ready: %s\n", cubeHttpServer.ready() ? "YES" : "NO");
  if (cubeHttpServer.ready()) {
    Serial.printf("Active Wi-Fi mode: %s\n",
                  cubeHttpServer.accessPointMode() ? "ap" : "sta");
    Serial.printf("URL: https://%s/\n", cubeHttpServer.localIp().c_str());
  }
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.flush();
}

void printWifiStatus() {
  const network::WifiSettings settings = network::WifiSettingsStore::load();
  Serial.println();
  Serial.println("Persistent Wi-Fi settings");
  Serial.printf("Configured mode: %s\n",
                network::WifiSettingsStore::modeName(settings.mode));
  Serial.printf("Stored station credentials: %s\n",
                settings.stationCredentialsStored ? "YES" : "NO");
  if (settings.stationCredentialsStored) {
    Serial.printf("Stored station SSID: %s\n", settings.stationSsid.c_str());
  }
  Serial.printf("Default AP SSID: %s\n",
                network::WifiSettingsStore::defaultApSsid().c_str());
  Serial.printf("Active mode: %s\n",
                cubeHttpServer.ready()
                    ? (cubeHttpServer.accessPointMode() ? "ap" : "sta")
                    : "not-started");
  Serial.printf("Active IP: %s\n",
                cubeHttpServer.ready() ? cubeHttpServer.localIp().c_str() : "-");
  Serial.flush();
}

void runWifiMode(const char* mode) {
  network::WifiMode target;
  if (std::strcmp(mode, "sta") == 0) {
    target = network::WifiMode::Station;
  } else if (std::strcmp(mode, "ap") == 0) {
    target = network::WifiMode::AccessPoint;
  } else {
    Serial.println("Usage: wifi-mode sta|ap");
    return;
  }

  const bool saved = network::WifiSettingsStore::saveMode(target);
  Serial.printf("Wi-Fi mode save: %s (%s)\n", saved ? "PASS" : "FAIL",
                network::WifiSettingsStore::modeName(target));
  if (saved) Serial.println("Run `reboot` to apply the new Wi-Fi mode.");
  Serial.flush();
}

void runWifiSet(const char* arguments) {
  const char* separator = std::strchr(arguments, '|');
  if (!separator) {
    Serial.println("Usage: wifi-set <ssid>|<password>");
    Serial.println("Example: wifi-set MyWifi|secret123");
    return;
  }

  String ssid(arguments, static_cast<unsigned int>(separator - arguments));
  String password(separator + 1);
  ssid.trim();

  if (ssid.isEmpty() || ssid.length() > 32) {
    Serial.println("Wi-Fi SSID must be 1..32 characters");
    return;
  }
  if (!password.isEmpty() && (password.length() < 8 || password.length() > 63)) {
    Serial.println("Wi-Fi password must be empty or 8..63 characters");
    return;
  }

  const bool saved = network::WifiSettingsStore::saveStationCredentials(
      ssid.c_str(), password.c_str());
  Serial.printf("Station credentials save: %s ssid=%s password=%s\n",
                saved ? "PASS" : "FAIL", ssid.c_str(),
                password.isEmpty() ? "<open>" : "<hidden>");
  if (saved) Serial.println("Use `wifi-mode sta`, then `reboot`, to connect with these credentials.");
  Serial.flush();
}

void printRobotResult(const char* action, bool result) {
  Serial.printf("%s: %s\n", action, result ? "PASS" : "FAIL");
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.flush();
}

void runMoveConvert(const char* sequence) {
  Serial.println();
  Serial.printf("Move input: %s\n", sequence);
  move::RobotMoveSequence converted{};
  if (!moveManager.convertText(sequence, converted, &Serial)) {
    Serial.println("Move conversion: FAIL");
    Serial.flush();
    return;
  }
  moveManager.printRobotSequence(converted, Serial);
  Serial.println("Move conversion: PASS");
  Serial.flush();
}

void runMoveAsync(const char* sequence) {
  Serial.println();
  Serial.printf("Async move input: %s\n", sequence);
  const bool accepted = moveManager.startText(sequence, Serial);
  Serial.printf("Move start: %s\n", accepted ? "ACCEPTED" : "REJECTED");
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.flush();
}

void runMoveBlocking(const char* sequence) {
  Serial.println();
  Serial.println("WARNING: blocking Serial diagnostic runner; HTTP is not serviced during servo delays.");
  Serial.printf("Move input: %s\n", sequence);
  const bool passed = moveManager.runTextBlocking(sequence, Serial);
  Serial.printf("Move run: %s\n", passed ? "PASS" : "FAIL");
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.flush();
}

void runServoSet(const char* arguments) {
  if (moveManager.isBusy()) {
    Serial.println("Servo calibration change rejected: runner is busy");
    return;
  }

  char key[32]{};
  int angle = 0;
  char extra = '\0';
  if (std::sscanf(arguments, "%31s %d %c", key, &angle, &extra) != 2) {
    Serial.println("Usage: servo-set <key> <degrees>");
    Serial.println("Keys: stand-correct, stand-turn, arm-pull, arm-hold, arm-release, arm-ready");
    return;
  }

  const bool changed = cubeRobot.setCalibration(key, angle, Serial);
  Serial.printf("Servo calibration set: %s\n", changed ? "PASS" : "FAIL");
  Serial.flush();
}

void runSolverSelfTest() {
  Serial.println();
  Serial.println("Solver integration metrics");
  Serial.printf("Heap before solver bind: %u bytes\n", ESP.getFreeHeap());
  if (!ensureSolverReady()) return;
  const uint32_t beforeTest = ESP.getFreeHeap();
  const bool passed = min2phaseSolver.selfTest(Serial);
  const uint32_t afterTest = ESP.getFreeHeap();
  Serial.printf("Heap before self-test: %u bytes\n", beforeTest);
  Serial.printf("Heap after self-test: %u bytes\n", afterTest);
  Serial.printf("Minimum free 8-bit heap: %u bytes\n",
                heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT));
  Serial.printf("Min2Phase solver self-test: %s\n", passed ? "PASS" : "FAIL");
  Serial.flush();
}

void runSolverBenchmark() {
  Serial.println();
  Serial.println("Representative solver benchmark");
  Serial.printf("Cases: %u fixed 20-move scrambles\n",
                static_cast<unsigned>(sizeof(kSolverBenchmarkCases) /
                                      sizeof(kSolverBenchmarkCases[0])));
  Serial.printf("Web server active: %s\n", cubeHttpServer.ready() ? "YES" : "NO");
  Serial.printf("Heap before benchmark: %u bytes\n", ESP.getFreeHeap());
  Serial.flush();

  if (!ensureSolverReady()) return;

  uint32_t totalMs = 0;
  uint32_t minMs = std::numeric_limits<uint32_t>::max();
  uint32_t maxMs = 0;
  uint8_t passed = 0;

  for (size_t i = 0; i < sizeof(kSolverBenchmarkCases) /
                              sizeof(kSolverBenchmarkCases[0]);
       ++i) {
    const auto& benchmark = kSolverBenchmarkCases[i];
    Serial.println();
    Serial.printf("Benchmark case %u BEGIN\n", static_cast<unsigned>(i + 1));
    Serial.printf("Scramble: %s\n", benchmark.scramble);
    Serial.flush();

    const solver::Min2PhaseSolveResult result =
        min2phaseSolver.solve(benchmark.facelets, 21);
    if (!result.success) {
      Serial.printf("Benchmark case %u: FAIL error=%d time=%u ms\n",
                    static_cast<unsigned>(i + 1), result.error,
                    static_cast<unsigned>(result.elapsedMs));
      Serial.flush();
      continue;
    }

    ++passed;
    totalMs += result.elapsedMs;
    minMs = std::min(minMs, result.elapsedMs);
    maxMs = std::max(maxMs, result.elapsedMs);
    Serial.printf("Benchmark case %u: PASS time=%u ms length=%u\n",
                  static_cast<unsigned>(i + 1),
                  static_cast<unsigned>(result.elapsedMs),
                  static_cast<unsigned>(result.length));
    Serial.printf("Solution: %s\n", result.moves);
    Serial.flush();
  }

  Serial.println();
  Serial.println("Representative solver benchmark summary");
  Serial.printf("Passed: %u/%u\n", static_cast<unsigned>(passed),
                static_cast<unsigned>(sizeof(kSolverBenchmarkCases) /
                                      sizeof(kSolverBenchmarkCases[0])));
  if (passed > 0) {
    Serial.printf("Solve min: %u ms\n", static_cast<unsigned>(minMs));
    Serial.printf("Solve average: %u ms\n",
                  static_cast<unsigned>(totalMs / passed));
    Serial.printf("Solve max: %u ms\n", static_cast<unsigned>(maxMs));
  }
  Serial.printf("Heap after benchmark: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("Minimum free 8-bit heap: %u bytes\n",
                heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT));
  Serial.flush();
}

bool commandStartsWith(const char* command, const char* prefix) {
  return std::strncmp(command, prefix, std::strlen(prefix)) == 0;
}

void executeSerialCommand(const char* command) {
  if (std::strcmp(command, "solver-test") == 0) {
    runSolverSelfTest();
  } else if (std::strcmp(command, "solver-bench") == 0) {
    runSolverBenchmark();
  } else if (std::strcmp(command, "solver-status") == 0) {
    printSolverStatus();
  } else if (std::strcmp(command, "sequence-self-test") == 0) {
    sequenceGenerator.selfTest(Serial);
  } else if (std::strcmp(command, "web-status") == 0) {
    printWebStatus();
  } else if (std::strcmp(command, "wifi-status") == 0) {
    printWifiStatus();
  } else if (commandStartsWith(command, "wifi-mode ")) {
    runWifiMode(command + std::strlen("wifi-mode "));
  } else if (commandStartsWith(command, "wifi-set ")) {
    runWifiSet(command + std::strlen("wifi-set "));
  } else if (std::strcmp(command, "wifi-clear") == 0) {
    const bool cleared = network::WifiSettingsStore::clearStationCredentials();
    Serial.printf("Stored station credentials clear: %s\n", cleared ? "PASS" : "NO_CHANGE");
    Serial.println("Reboot to apply fallback/AP behavior.");
  } else if (std::strcmp(command, "reboot") == 0) {
    if (moveManager.isBusy()) {
      Serial.println("Reboot rejected: runner is busy");
    } else {
      Serial.println("Rebooting...");
      Serial.flush();
      delay(100);
      ESP.restart();
    }
  } else if (std::strcmp(command, "move-self-test") == 0) {
    moveManager.selfTest(Serial);
  } else if (commandStartsWith(command, "move-convert ")) {
    runMoveConvert(command + std::strlen("move-convert "));
  } else if (commandStartsWith(command, "move-start ")) {
    runMoveAsync(command + std::strlen("move-start "));
  } else if (std::strcmp(command, "move-status") == 0) {
    moveManager.printRunnerStatus(Serial);
  } else if (std::strcmp(command, "move-stop") == 0) {
    moveManager.requestStop();
    moveManager.printRunnerStatus(Serial);
  } else if (commandStartsWith(command, "move-run ")) {
    runMoveBlocking(command + std::strlen("move-run "));
  } else if (commandStartsWith(command, "run ")) {
    runMoveBlocking(command + std::strlen("run "));
  } else if (std::strcmp(command, "servo-status") == 0) {
    cubeRobot.printStatus(Serial);
  } else if (std::strcmp(command, "servo-config") == 0) {
    cubeRobot.printCalibration(Serial);
  } else if (commandStartsWith(command, "servo-set ")) {
    runServoSet(command + std::strlen("servo-set "));
  } else if (std::strcmp(command, "servo-init") == 0) {
    printRobotResult("servo-init", cubeRobot.init(Serial));
  } else if (std::strcmp(command, "servo-off") == 0) {
    cubeRobot.stop(Serial);
  } else if (std::strcmp(command, "robot-d") == 0) {
    printRobotResult("robot-d", cubeRobot.d(1, Serial));
  } else if (std::strcmp(command, "robot-d2") == 0) {
    printRobotResult("robot-d2", cubeRobot.d(2, Serial));
  } else if (std::strcmp(command, "robot-dp") == 0) {
    printRobotResult("robot-dp", cubeRobot.dp(1, Serial));
  } else if (std::strcmp(command, "robot-dp2") == 0) {
    printRobotResult("robot-dp2", cubeRobot.dp(2, Serial));
  } else if (std::strcmp(command, "robot-x") == 0) {
    printRobotResult("robot-x", cubeRobot.x(1, Serial));
  } else if (std::strcmp(command, "robot-x2") == 0) {
    printRobotResult("robot-x2", cubeRobot.x(2, Serial));
  } else if (std::strcmp(command, "robot-x3") == 0) {
    printRobotResult("robot-x3", cubeRobot.x(3, Serial));
  } else if (std::strcmp(command, "robot-y") == 0) {
    printRobotResult("robot-y", cubeRobot.y(1, Serial));
  } else if (std::strcmp(command, "robot-y2") == 0) {
    printRobotResult("robot-y2", cubeRobot.y(2, Serial));
  } else if (std::strcmp(command, "robot-yp") == 0) {
    printRobotResult("robot-yp", cubeRobot.yp(1, Serial));
  } else if (std::strcmp(command, "robot-yp2") == 0) {
    printRobotResult("robot-yp2", cubeRobot.yp(2, Serial));
  } else if (std::strcmp(command, "help") == 0) {
    printHelp();
  } else if (command[0] != '\0') {
    Serial.printf("Unknown command: %s\n", command);
    printHelp();
  }
}

void pollSerialCommands() {
  while (Serial.available() > 0) {
    const char ch = static_cast<char>(Serial.read());
    if (ch == '\r' || ch == '\n') {
      if (serialCommandLength > 0) {
        serialCommandBuffer[serialCommandLength] = '\0';
        executeSerialCommand(serialCommandBuffer);
        serialCommandLength = 0;
      }
      continue;
    }

    if (serialCommandLength + 1 < kSerialCommandBufferSize) {
      serialCommandBuffer[serialCommandLength++] = ch;
    } else {
      serialCommandLength = 0;
      Serial.println("Serial command too long; buffer cleared");
      Serial.flush();
    }
  }
}
}  // namespace

void setup() {
  Serial.begin(kSerialBaud);
  // USB Serial is diagnostic-only. Do not wait for a host connection: normal
  // appliance startup must proceed immediately when powered without a PC.
  hardwareControls.begin(&Serial);
  printSystemInfo();

  if (solverTableStorage.begin(Serial)) {
    realTablesReady = printTableImageStatus();
  }

  if (realTablesReady && ensureSolverReady()) {
    cubeHttpServer.begin(min2phaseSolver, moveManager, sequenceGenerator, Serial);
  }

  // CubeRobot is initialized by the standalone Web bootstrap before controls
  // are exposed. Serial remains available afterward for diagnostics.
  printHelp();
  Serial.flush();
}

void loop() {
  cubeHttpServer.handleClient();
  hardwareControls.poll(&Serial);
  pollSerialCommands();
  delay(1);
}