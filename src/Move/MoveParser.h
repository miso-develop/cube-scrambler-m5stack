#pragma once

#include <Arduino.h>

#include "MoveTypes.h"

namespace move {

class MoveParser {
 public:
  static bool parseSequence(const char* sequence, MoveSequence& out, Stream* log = nullptr);
  static bool parseMoveToken(const char* token, MoveToken& out);
  static bool parseRobotMoveToken(const char* token, RobotMoveToken& out);

  static size_t formatMove(const MoveToken& move, char* out, size_t outSize);
  static size_t formatRobotMove(const RobotMoveToken& move, char* out, size_t outSize);

 private:
  static bool isBasicPosition(char position);
  static bool isSlicePosition(char position);
  static bool isRotationPosition(char position);
};

}  // namespace move
