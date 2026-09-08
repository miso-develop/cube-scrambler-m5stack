#include "MoveRunner.h"

#include "MoveParser.h"

namespace move {

bool MoveRunner::canStart() const {
  return state_ != State::Running && state_ != State::Stopping;
}

bool MoveRunner::isBusy() const {
  return state_ == State::Running || state_ == State::Stopping;
}

const char* MoveRunner::stateName(State state) {
  switch (state) {
    case State::Waiting:
      return "idle";
    case State::Running:
      return "running";
    case State::Stopping:
      return "stopping";
    case State::Finished:
      return "finished";
    case State::Stopped:
      return "stopped";
    case State::Error:
      return "error";
  }
  return "error";
}

MoveRunner::Status MoveRunner::status() const {
  Status result{};
  result.state = state_;
  result.currentMove = currentMove_;
  result.totalMoves = totalMoves_;
  result.startedMs = startedMs_;
  result.finishedMs = finishedMs_;
  result.stopRequested = stopRequested_;
  if (workerTask_) {
    result.workerStackHighWater =
        static_cast<uint32_t>(uxTaskGetStackHighWaterMark(workerTask_));
  }
  return result;
}

bool MoveRunner::ensureWorker(Stream& out) {
  if (workerTask_) return true;

  const BaseType_t created =
      xTaskCreate(&MoveRunner::workerEntry, "cube-move", kWorkerStackSize, this,
                  kWorkerPriority, &workerTask_);
  if (created != pdPASS || !workerTask_) {
    out.println("Move runner failed: could not create worker task");
    out.flush();
    workerTask_ = nullptr;
    return false;
  }

  out.printf("Move worker: READY stack=%u\n",
             static_cast<unsigned>(kWorkerStackSize));
  out.flush();
  return true;
}

bool MoveRunner::start(const RobotMoveSequence& moves, Stream& out) {
  if (!cubeRobot_.ready()) {
    out.println("Move start failed: CubeRobot not initialized. Run servo-init first.");
    out.flush();
    return false;
  }
  if (moves.count == 0 || moves.count > kMaxRobotMoves) {
    out.println("Move start failed: invalid robot move count");
    out.flush();
    return false;
  }
  if (!canStart()) {
    out.println("Move start failed: runner is busy");
    out.flush();
    return false;
  }
  if (!ensureWorker(out)) return false;

  // The worker is blocked on a task notification while canStart() is true, so
  // this fixed-size copy completes before it can read pending_.
  pending_ = moves;
  out_ = &out;
  stopRequested_ = false;
  currentMove_ = 0;
  totalMoves_ = moves.count;
  startedMs_ = millis();
  finishedMs_ = 0;
  state_ = State::Running;

  xTaskNotifyGive(workerTask_);
  return true;
}

void MoveRunner::requestStop() {
  if (!isBusy()) return;
  stopRequested_ = true;
  state_ = State::Stopping;
}

void MoveRunner::workerEntry(void* context) {
  static_cast<MoveRunner*>(context)->workerLoop();
}

void MoveRunner::workerLoop() {
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    bool stopped = false;
    const bool ok = executePending(stopped);
    finishedMs_ = millis();

    if (stopped) {
      state_ = State::Stopped;
    } else if (ok) {
      state_ = State::Finished;
    } else {
      state_ = State::Error;
    }
  }
}

bool MoveRunner::executeMove(const RobotMoveToken& move, Stream& out) {
  switch (move.position) {
    case 'D':
      return move.prime ? cubeRobot_.dp(move.count, out)
                        : cubeRobot_.d(move.count, out);
    case 'x':
      return !move.prime && cubeRobot_.x(move.count, out);
    case 'y':
      return move.prime ? cubeRobot_.yp(move.count, out)
                        : cubeRobot_.y(move.count, out);
    default:
      return false;
  }
}

bool MoveRunner::executePending(bool& stopped) {
  stopped = false;
  if (!out_) return false;
  Stream& out = *out_;

  out.printf("Robot sequence async BEGIN (%u moves)\n",
             static_cast<unsigned>(pending_.count));
  out.flush();

  auto parkArmForStop = [&]() {
    out.println("Robot sequence stop: moving Arm to ready position");
    out.flush();
    if (!cubeRobot_.park(out)) {
      out.println("Robot sequence stop FAIL: Arm ready failed");
      out.flush();
      return false;
    }
    stopped = true;
    out.println("Robot sequence stop: Arm ready complete");
    out.flush();
    return true;
  };

  for (size_t i = 0; i < pending_.count; ++i) {
    if (stopRequested_) {
      out.printf("Robot sequence STOPPED before move %u\n",
                 static_cast<unsigned>(i + 1));
      out.flush();
      parkArmForStop();
      return false;
    }

    currentMove_ = i + 1;
    const RobotMoveToken& move = pending_.items[i];
    char notation[4]{};
    MoveParser::formatRobotMove(move, notation, sizeof(notation));
    out.printf("  [%u/%u] %s\n", static_cast<unsigned>(i + 1),
               static_cast<unsigned>(pending_.count), notation);
    out.flush();

    if (!executeMove(move, out)) {
      out.printf("Robot sequence async FAIL at %s\n", notation);
      out.flush();
      return false;
    }

    if (stopRequested_) {
      out.printf("Robot sequence STOPPED after move %u\n",
                 static_cast<unsigned>(i + 1));
      out.flush();
      parkArmForStop();
      return false;
    }
  }

  if (!cubeRobot_.park(out)) {
    out.println("Robot sequence async FAIL while parking arm");
    out.flush();
    return false;
  }

  currentMove_ = pending_.count;
  out.println("Robot sequence async PASS");
  out.flush();
  return true;
}

bool MoveRunner::runBlocking(const RobotMoveSequence& moves, Stream& out) {
  if (!cubeRobot_.ready()) {
    out.println("Move run failed: CubeRobot not initialized. Run servo-init first.");
    out.flush();
    return false;
  }
  if (!canStart()) {
    out.println("Move run failed: runner is busy");
    out.flush();
    return false;
  }

  pending_ = moves;
  out_ = &out;
  stopRequested_ = false;
  currentMove_ = 0;
  totalMoves_ = moves.count;
  startedMs_ = millis();
  finishedMs_ = 0;
  state_ = State::Running;

  bool stopped = false;
  const bool ok = executePending(stopped);
  finishedMs_ = millis();
  state_ = stopped ? State::Stopped : (ok ? State::Finished : State::Error);
  return ok;
}

}  // namespace move
