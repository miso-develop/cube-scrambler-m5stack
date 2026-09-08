#include "MoveConverter.h"

#include <cctype>
#include <cstring>

#include "FaceOrientation.h"
#include "MoveParser.h"

namespace move {

bool MoveConverter::convert(const MoveSequence& moves, RobotMoveSequence& out,
                            Stream* log) {
  out.count = 0;
  cubeState_ = CubeState{};
  robotCubeState_ = CubeState{};

  for (size_t i = 0; i < moves.count; ++i) {
    if (!convertOne(moves.items[i], out, log)) return false;
  }

  // Existing runtime config has LAST_CORRECT_ORIENTATION=false. Preserve that
  // behavior for the standalone port; final orientation correction is not
  // appended automatically.
  return true;
}

bool MoveConverter::convertOne(const MoveToken& move, RobotMoveSequence& out,
                               Stream* log) {
  switch (move.kind) {
    case MoveKind::Basic:
      return convertBasic(move, out, log);
    case MoveKind::Wide:
      return convertWide(move, out, log);
    case MoveKind::Slice:
      return convertSlice(move, out, log);
    case MoveKind::Rotation:
      applyLogicalRotation(move.position, move.prime, move.count);
      return true;
  }
  return false;
}

char MoveConverter::relativeBasicPosition(char position) const {
  const char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(position)));
  const Face logicalFace = FaceOrientation::faceAt(cubeState_, lower);
  const char robotPosition =
      FaceOrientation::findFacePosition(robotCubeState_, logicalFace);
  return robotPosition == '\0'
             ? '\0'
             : static_cast<char>(std::toupper(static_cast<unsigned char>(robotPosition)));
}

bool MoveConverter::convertBasic(const MoveToken& move, RobotMoveSequence& out,
                                 Stream* log) {
  const char relative = relativeBasicPosition(move.position);
  if (relative == '\0') {
    if (log) log->println("Move convert failed: relative face not found");
    return false;
  }
  return appendBasicPlan(relative, move.prime, move.count, out, log);
}

bool MoveConverter::convertWide(const MoveToken& move, RobotMoveSequence& out,
                                Stream* log) {
  char basicPosition = '\0';
  bool rotationPrime = false;
  char rotationAxis = '\0';

  switch (move.position) {
    case 'U':
      basicPosition = 'D';
      rotationAxis = 'y';
      rotationPrime = move.prime;
      break;
    case 'D':
      basicPosition = 'U';
      rotationAxis = 'y';
      rotationPrime = !move.prime;
      break;
    case 'R':
      basicPosition = 'L';
      rotationAxis = 'x';
      rotationPrime = move.prime;
      break;
    case 'L':
      basicPosition = 'R';
      rotationAxis = 'x';
      rotationPrime = !move.prime;
      break;
    case 'F':
      basicPosition = 'B';
      rotationAxis = 'z';
      rotationPrime = move.prime;
      break;
    case 'B':
      basicPosition = 'F';
      rotationAxis = 'z';
      rotationPrime = !move.prime;
      break;
    default:
      if (log) log->println("Move convert failed: invalid wide move");
      return false;
  }

  // Mirror the original converter: the Basic move produced by a Wide move is
  // converted directly; the logical cube rotation is tracked separately.
  if (!appendBasicPlan(basicPosition, move.prime, move.count, out, log)) {
    return false;
  }
  applyLogicalRotation(rotationAxis, rotationPrime, move.count);
  return true;
}

bool MoveConverter::convertSlice(const MoveToken& move, RobotMoveSequence& out,
                                 Stream* log) {
  char firstPosition = '\0';
  char secondPosition = '\0';
  bool firstPrime = false;
  bool secondPrime = false;
  char rotationAxis = '\0';
  bool rotationPrime = false;

  switch (move.position) {
    case 'M':
      firstPosition = 'R';
      secondPosition = 'L';
      firstPrime = move.prime;
      secondPrime = !move.prime;
      rotationAxis = 'x';
      rotationPrime = !move.prime;
      break;
    case 'E':
      firstPosition = 'U';
      secondPosition = 'D';
      firstPrime = move.prime;
      secondPrime = !move.prime;
      rotationAxis = 'y';
      rotationPrime = !move.prime;
      break;
    case 'S':
      firstPosition = 'F';
      secondPosition = 'B';
      firstPrime = !move.prime;
      secondPrime = move.prime;
      rotationAxis = 'z';
      rotationPrime = move.prime;
      break;
    default:
      if (log) log->println("Move convert failed: invalid slice move");
      return false;
  }

  const char firstRelative = relativeBasicPosition(firstPosition);
  if (firstRelative == '\0' ||
      !appendBasicPlan(firstRelative, firstPrime, move.count, out, log)) {
    return false;
  }

  // The first physical conversion changes robotCubeState_, therefore the
  // second Basic move must resolve its relative position afterwards just like
  // the original TypeScript implementation.
  const char secondRelative = relativeBasicPosition(secondPosition);
  if (secondRelative == '\0' ||
      !appendBasicPlan(secondRelative, secondPrime, move.count, out, log)) {
    return false;
  }

  applyLogicalRotation(rotationAxis, rotationPrime, move.count);
  return true;
}

bool MoveConverter::appendBasicPlan(char position, bool prime, uint8_t count,
                                    RobotMoveSequence& out, Stream* log) {
  switch (position) {
    case 'U':
      if (!appendRobot('x', false, 2, out, log)) return false;
      break;
    case 'R':
      if (!appendRobot('y', true, 1, out, log)) return false;
      if (!appendRobot('x', false, 1, out, log)) return false;
      break;
    case 'L':
      if (!appendRobot('y', false, 1, out, log)) return false;
      if (!appendRobot('x', false, 1, out, log)) return false;
      break;
    case 'F':
      if (!appendRobot('x', false, 3, out, log)) return false;
      break;
    case 'B':
      if (!appendRobot('x', false, 1, out, log)) return false;
      break;
    case 'D':
      break;
    default:
      if (log) log->printf("Move convert failed: invalid Basic face '%c'\n", position);
      return false;
  }

  return appendRobot('D', prime, count, out, log);
}

bool MoveConverter::appendRobot(char position, bool prime, uint8_t count,
                                RobotMoveSequence& out, Stream* log) {
  if (out.count >= kMaxRobotMoves) {
    if (log) log->println("Move convert failed: robot sequence overflow");
    return false;
  }

  RobotMoveToken robotMove{position, prime, count};
  char token[4]{};
  MoveParser::formatRobotMove(robotMove, token, sizeof(token));
  RobotMoveToken validated{};
  if (!MoveParser::parseRobotMoveToken(token, validated)) {
    if (log) log->printf("Move convert failed: invalid robot move '%s'\n", token);
    return false;
  }

  out.items[out.count++] = validated;

  if (position == 'x') {
    robotCubeState_ = FaceOrientation::rotateX(robotCubeState_, count);
  } else if (position == 'y') {
    const int signedCount = prime ? -static_cast<int>(count)
                                  : static_cast<int>(count);
    robotCubeState_ = FaceOrientation::rotateY(robotCubeState_, signedCount);
  }
  return true;
}

void MoveConverter::applyLogicalRotation(char axis, bool prime, uint8_t count) {
  const int signedCount = prime ? -static_cast<int>(count)
                                : static_cast<int>(count);
  switch (axis) {
    case 'x':
      cubeState_ = FaceOrientation::rotateX(cubeState_, signedCount);
      break;
    case 'y':
      cubeState_ = FaceOrientation::rotateY(cubeState_, signedCount);
      break;
    case 'z':
      cubeState_ = FaceOrientation::rotateZ(cubeState_, signedCount);
      break;
  }
}

bool MoveConverter::sequenceEquals(const RobotMoveSequence& actual,
                                   const char* expected) const {
  MoveSequence ignored{};
  (void)ignored;

  const char* cursor = expected;
  size_t index = 0;
  while (*cursor != '\0') {
    while (*cursor == ' ') ++cursor;
    if (*cursor == '\0') break;

    char token[4]{};
    size_t length = 0;
    while (*cursor != '\0' && *cursor != ' ' && length + 1 < sizeof(token)) {
      token[length++] = *cursor++;
    }
    token[length] = '\0';

    if (index >= actual.count) return false;
    RobotMoveToken parsed{};
    if (!MoveParser::parseRobotMoveToken(token, parsed)) return false;
    const RobotMoveToken& value = actual.items[index++];
    if (value.position != parsed.position || value.prime != parsed.prime ||
        value.count != parsed.count) {
      return false;
    }
  }
  return index == actual.count;
}

bool MoveConverter::selfTest(Stream& out) {
  struct Case {
    const char* input;
    const char* expected;
  };
  constexpr Case cases[] = {
      {"U", "x2 D"},
      {"R", "y' x D"},
      {"F", "x3 D"},
      {"D2", "D2"},
      {"M", "y' x D x2 D'"},
      {"Uw", "D"},
      {"x", ""},
  };

  out.println();
  out.println("Move converter self-test");
  bool allPassed = true;
  for (const auto& test : cases) {
    MoveSequence parsed{};
    RobotMoveSequence converted{};
    const bool passed = MoveParser::parseSequence(test.input, parsed, &out) &&
                        convert(parsed, converted, &out) &&
                        sequenceEquals(converted, test.expected);
    out.printf("  %-4s -> %s\n", test.input, passed ? "PASS" : "FAIL");
    allPassed = allPassed && passed;
  }
  out.printf("Move converter self-test: %s\n", allPassed ? "PASS" : "FAIL");
  out.flush();
  return allPassed;
}

}  // namespace move
