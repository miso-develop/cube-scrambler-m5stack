#pragma once

#include "MoveTypes.h"

namespace move {

class FaceOrientation {
 public:
  static Face reverseFace(Face face);
  static Face faceAt(const CubeState& state, char position);
  static char findFacePosition(const CubeState& state, Face face);

  static CubeState rotateX(const CubeState& state, int count);
  static CubeState rotateY(const CubeState& state, int count);
  static CubeState rotateZ(const CubeState& state, int count);

 private:
  static int normalizeQuarterTurns(int count);
};

}  // namespace move
