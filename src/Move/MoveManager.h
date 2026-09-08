#pragma once

#include <Arduino.h>

#include "MoveConverter.h"
#include "MoveParser.h"
#include "MoveRunner.h"
#include "MoveTypes.h"
#include "../Robot/CubeRobot.h"

namespace move {

class MoveManager {
 public:
  explicit MoveManager(robot::CubeRobot& cubeRobot)
      : cubeRobot_(cubeRobot), runner_(cubeRobot) {}

  bool parseAndConvert(const char* sequence, MoveSequence& parsed,
                       RobotMoveSequence& converted, Stream* log = nullptr);
  bool convertText(const char* sequence, RobotMoveSequence& converted,
                   Stream* log = nullptr);

  bool initializeRobot(Stream& out) {
    return cubeRobot_.ready() || cubeRobot_.init(out);
  }
  bool startConverted(const RobotMoveSequence& converted, Stream& out);
  bool startText(const char* sequence, Stream& out);
  void requestStop() { runner_.requestStop(); }

  bool runTextBlocking(const char* sequence, Stream& out);

  void printRobotSequence(const RobotMoveSequence& sequence, Stream& out) const;
  void printRunnerStatus(Stream& out) const;
  bool selfTest(Stream& out);

  MoveRunner::State runnerState() const { return runner_.state(); }
  MoveRunner::Status runnerStatus() const { return runner_.status(); }
  bool canStart() const { return runner_.canStart(); }
  bool isBusy() const { return runner_.isBusy(); }
  bool robotReady() const { return cubeRobot_.ready(); }
  robot::CubeRobot& robot() { return cubeRobot_; }
  const robot::CubeRobot& robot() const { return cubeRobot_; }

 private:
  robot::CubeRobot& cubeRobot_;
  MoveConverter converter_;
  MoveRunner runner_;
};

}  // namespace move
