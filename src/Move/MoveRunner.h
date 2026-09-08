#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "MoveTypes.h"
#include "../Robot/CubeRobot.h"

namespace move {

class MoveRunner {
 public:
  enum class State : uint8_t {
    Waiting,
    Running,
    Stopping,
    Finished,
    Stopped,
    Error,
  };

  struct Status {
    State state = State::Waiting;
    size_t currentMove = 0;
    size_t totalMoves = 0;
    uint32_t startedMs = 0;
    uint32_t finishedMs = 0;
    bool stopRequested = false;
    uint32_t workerStackHighWater = 0;
  };

  explicit MoveRunner(robot::CubeRobot& cubeRobot) : cubeRobot_(cubeRobot) {}

  // Starts the sequence on a dedicated FreeRTOS worker task. Servo methods keep
  // their calibrated delay() calls; while the worker sleeps the Arduino loop
  // task can continue servicing HTTP and Serial.
  bool start(const RobotMoveSequence& moves, Stream& out);
  void requestStop();

  // Diagnostic fallback. Production HTTP execution uses start().
  bool runBlocking(const RobotMoveSequence& moves, Stream& out);

  State state() const { return state_; }
  Status status() const;
  bool canStart() const;
  bool isBusy() const;

  static const char* stateName(State state);

 private:
  static constexpr uint32_t kWorkerStackSize = 6144;
  static constexpr UBaseType_t kWorkerPriority = 1;

  bool ensureWorker(Stream& out);
  static void workerEntry(void* context);
  void workerLoop();
  bool executePending(bool& stopped);
  bool executeMove(const RobotMoveToken& move, Stream& out);

  robot::CubeRobot& cubeRobot_;
  TaskHandle_t workerTask_ = nullptr;
  RobotMoveSequence pending_{};
  Stream* out_ = nullptr;

  volatile State state_ = State::Waiting;
  volatile bool stopRequested_ = false;
  volatile size_t currentMove_ = 0;
  volatile size_t totalMoves_ = 0;
  volatile uint32_t startedMs_ = 0;
  volatile uint32_t finishedMs_ = 0;
};

}  // namespace move
