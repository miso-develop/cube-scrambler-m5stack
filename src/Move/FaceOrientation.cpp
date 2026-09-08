#include "FaceOrientation.h"

namespace move {

Face FaceOrientation::reverseFace(Face face) {
  switch (face) {
    case Face::U: return Face::D;
    case Face::R: return Face::L;
    case Face::F: return Face::B;
    case Face::D: return Face::U;
    case Face::L: return Face::R;
    case Face::B: return Face::F;
  }
  return Face::U;
}

Face FaceOrientation::faceAt(const CubeState& state, char position) {
  switch (position) {
    case 'u': return state.u;
    case 'r': return state.r;
    case 'f': return state.f;
    case 'd': return reverseFace(state.u);
    case 'l': return reverseFace(state.r);
    case 'b': return reverseFace(state.f);
    default: return Face::U;
  }
}

char FaceOrientation::findFacePosition(const CubeState& state, Face face) {
  constexpr char positions[] = {'u', 'r', 'f', 'd', 'l', 'b'};
  for (char position : positions) {
    if (faceAt(state, position) == face) return position;
  }
  return '\0';
}

int FaceOrientation::normalizeQuarterTurns(int count) {
  int normalized = count % 4;
  if (normalized < 0) normalized += 4;
  return normalized;
}

CubeState FaceOrientation::rotateX(const CubeState& state, int count) {
  const Face line[4] = {
      state.u,
      state.f,
      reverseFace(state.u),
      reverseFace(state.f),
  };
  const int offset = normalizeQuarterTurns(count);
  CubeState result = state;
  result.u = line[offset];
  result.f = line[(offset + 1) % 4];
  return result;
}

CubeState FaceOrientation::rotateY(const CubeState& state, int count) {
  const Face line[4] = {
      state.f,
      state.r,
      reverseFace(state.f),
      reverseFace(state.r),
  };
  const int offset = normalizeQuarterTurns(count);
  CubeState result = state;
  result.f = line[offset];
  result.r = line[(offset + 1) % 4];
  return result;
}

CubeState FaceOrientation::rotateZ(const CubeState& state, int count) {
  const Face line[4] = {
      state.r,
      state.u,
      reverseFace(state.r),
      reverseFace(state.u),
  };
  const int offset = normalizeQuarterTurns(count);
  CubeState result = state;
  result.r = line[offset];
  result.u = line[(offset + 1) % 4];
  return result;
}

}  // namespace move
