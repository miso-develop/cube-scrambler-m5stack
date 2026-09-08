#pragma once

#include <Arduino.h>

#include "MoveTypes.h"

namespace move {

class MoveConverter {
 public:
  bool convert(const MoveSequence& moves, RobotMoveSequence& out,
               Stream* log = nullptr);
  bool selfTest(Stream& out);

 private:
  bool convertOne(const MoveToken& move, RobotMoveSequence& out, Stream* log);
  bool convertBasic(const MoveToken& move, RobotMoveSequence& out, Stream* log);
  bool convertWide(const MoveToken& move, RobotMoveSequence& out, Stream* log);
  bool convertSlice(const MoveToken& move, RobotMoveSequence& out, Stream* log);

  char relativeBasicPosition(char position) const;
  bool appendBasicPlan(char position, bool prime, uint8_t count,
                       RobotMoveSequence& out, Stream* log);
  bool appendRobot(char position, bool prime, uint8_t count,
                   RobotMoveSequence& out, Stream* log);
  void applyLogicalRotation(char axis, bool prime, uint8_t count);

  bool sequenceEquals(const RobotMoveSequence& actual,
                      const char* expected) const;

  CubeState cubeState_{};
  CubeState robotCubeState_{};
};

}  // namespace move
