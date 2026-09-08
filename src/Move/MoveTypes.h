#pragma once

#include <Arduino.h>

namespace move {

constexpr size_t kMaxMoves = 64;
constexpr size_t kMaxRobotMoves = 192;

enum class MoveKind : uint8_t {
  Basic,
  Slice,
  Wide,
  Rotation,
};

struct MoveToken {
  char position = '\0';
  bool prime = false;
  bool wide = false;
  uint8_t count = 1;
  MoveKind kind = MoveKind::Basic;
};

struct RobotMoveToken {
  char position = '\0';  // D / x / y
  bool prime = false;
  uint8_t count = 1;
};

struct MoveSequence {
  MoveToken items[kMaxMoves]{};
  size_t count = 0;
};

struct RobotMoveSequence {
  RobotMoveToken items[kMaxRobotMoves]{};
  size_t count = 0;
};

enum class Face : uint8_t {
  U,
  R,
  F,
  D,
  L,
  B,
};

struct CubeState {
  Face u = Face::U;
  Face r = Face::R;
  Face f = Face::F;
};

}  // namespace move
