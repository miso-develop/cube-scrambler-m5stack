#include "MoveParser.h"

#include <cctype>
#include <cstring>

namespace move {

bool MoveParser::isBasicPosition(char position) {
  return position == 'F' || position == 'U' || position == 'R' ||
         position == 'B' || position == 'D' || position == 'L';
}

bool MoveParser::isSlicePosition(char position) {
  return position == 'M' || position == 'E' || position == 'S';
}

bool MoveParser::isRotationPosition(char position) {
  return position == 'x' || position == 'y' || position == 'z';
}

bool MoveParser::parseMoveToken(const char* token, MoveToken& out) {
  if (!token || token[0] == '\0') return false;

  const size_t length = std::strlen(token);
  if (length < 1 || length > 4) return false;

  MoveToken parsed{};
  parsed.position = token[0];
  size_t index = 1;

  if (index < length && token[index] == '\'') {
    parsed.prime = true;
    ++index;
  }
  if (index < length && token[index] == 'w') {
    parsed.wide = true;
    ++index;
  }
  if (index < length && token[index] == '2') {
    parsed.count = 2;
    ++index;
  }
  if (index != length) return false;

  if (isBasicPosition(parsed.position)) {
    parsed.kind = parsed.wide ? MoveKind::Wide : MoveKind::Basic;
  } else if (isSlicePosition(parsed.position) && !parsed.wide) {
    parsed.kind = MoveKind::Slice;
  } else if (isRotationPosition(parsed.position) && !parsed.wide) {
    parsed.kind = MoveKind::Rotation;
  } else {
    return false;
  }

  out = parsed;
  return true;
}

bool MoveParser::parseRobotMoveToken(const char* token, RobotMoveToken& out) {
  if (!token || token[0] == '\0') return false;

  const size_t length = std::strlen(token);
  if (length < 1 || length > 3) return false;

  RobotMoveToken parsed{};
  parsed.position = token[0];
  if (parsed.position != 'D' && parsed.position != 'x' && parsed.position != 'y') {
    return false;
  }

  size_t index = 1;
  if (index < length && token[index] == '\'') {
    parsed.prime = true;
    ++index;
  }
  if (index < length && token[index] >= '2' && token[index] <= '3') {
    parsed.count = static_cast<uint8_t>(token[index] - '0');
    ++index;
  }
  if (index != length) return false;

  if (parsed.position == 'D') {
    if (parsed.count > 2) return false;
  } else if (parsed.position == 'x') {
    if (parsed.prime || parsed.count > 3) return false;
  } else if (parsed.position == 'y') {
    if (parsed.count > 2) return false;
  }

  out = parsed;
  return true;
}

bool MoveParser::parseSequence(const char* sequence, MoveSequence& out, Stream* log) {
  out.count = 0;
  if (!sequence) return false;

  char token[5]{};
  size_t tokenLength = 0;

  auto flushToken = [&]() -> bool {
    if (tokenLength == 0) return true;
    token[tokenLength] = '\0';
    if (out.count >= kMaxMoves) {
      if (log) log->println("Move parse failed: too many moves");
      return false;
    }
    MoveToken parsed{};
    if (!parseMoveToken(token, parsed)) {
      if (log) log->printf("Move parse failed: invalid token '%s'\n", token);
      return false;
    }
    out.items[out.count++] = parsed;
    tokenLength = 0;
    token[0] = '\0';
    return true;
  };

  for (const char* cursor = sequence;; ++cursor) {
    const unsigned char raw = static_cast<unsigned char>(*cursor);
    const bool end = raw == 0;
    const bool whitespace = !end && std::isspace(raw);

    if (end || whitespace) {
      if (!flushToken()) return false;
      if (end) break;
      continue;
    }

    // Preserve the original parser's cleansing behavior.
    if (raw == '/' || raw == '|' || raw == '-' || raw == '"') continue;

    if (tokenLength + 1 >= sizeof(token)) {
      if (log) log->println("Move parse failed: token too long");
      return false;
    }
    token[tokenLength++] = static_cast<char>(raw);
  }

  if (out.count == 0) {
    if (log) log->println("Move parse failed: empty sequence");
    return false;
  }
  return true;
}

size_t MoveParser::formatMove(const MoveToken& move, char* out, size_t outSize) {
  if (!out || outSize == 0) return 0;
  size_t index = 0;
  auto append = [&](char ch) {
    if (index + 1 < outSize) out[index++] = ch;
  };
  append(move.position);
  if (move.prime) append('\'');
  if (move.wide) append('w');
  if (move.count == 2) append('2');
  out[index < outSize ? index : outSize - 1] = '\0';
  return index;
}

size_t MoveParser::formatRobotMove(const RobotMoveToken& move, char* out,
                                   size_t outSize) {
  if (!out || outSize == 0) return 0;
  size_t index = 0;
  auto append = [&](char ch) {
    if (index + 1 < outSize) out[index++] = ch;
  };
  append(move.position);
  if (move.prime) append('\'');
  if (move.count > 1) append(static_cast<char>('0' + move.count));
  out[index < outSize ? index : outSize - 1] = '\0';
  return index;
}

}  // namespace move
