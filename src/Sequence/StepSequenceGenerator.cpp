#include "StepSequenceGenerator.h"

#include <algorithm>
#include <cstring>

#include "RandomCubeGenerator.h"
#include "Solver/Min2PhaseSolver.h"

namespace sequence {
namespace {

constexpr uint32_t kGenerateTimeoutMs = 1000;

constexpr const char* kStepTemplates[9] = {
    nullptr,
    "------------------------------------------------------",
    "----------------R--------F--D-D-D-D--------L--------B-",
    "------------R-R-R----F-F-F--D-D-D-D----L-L-L----B-B-B-",
    "------------R-RRRR---F-FFFFDDDD-DDDD---L-LLLL---B-BBBB",
    "-U-U-U-U----R-RRRR---F-FFFFDDDD-DDDD---L-LLLL---B-BBBB",
    "UUUU-UUUU---R-RRRR---F-FFFFDDDD-DDDD---L-LLLL---B-BBBB",
    "UUUU-UUUUR-RR-RRRRF-FF-FFFFDDDD-DDDDL-LL-LLLLB-BB-BBBB",
    "UUUU-UUUURRRR-RRRRFFFF-FFFFDDDD-DDDDLLLL-LLLLBBBB-BBBB",
};

constexpr StepSequenceGenerator::PartPosition kCornerPositions[] = {
    {{0, 36, 47}, 3}, {{2, 11, 45}, 3}, {{6, 18, 38}, 3},
    {{8, 9, 20}, 3},  {{15, 26, 29}, 3}, {{17, 35, 51}, 3},
    {{24, 27, 44}, 3}, {{33, 42, 53}, 3},
};

constexpr StepSequenceGenerator::PartPosition kEdgePositions[] = {
    {{1, 46, 0}, 2},  {{3, 37, 0}, 2},  {{5, 10, 0}, 2},
    {{7, 19, 0}, 2},  {{12, 23, 0}, 2}, {{14, 48, 0}, 2},
    {{16, 32, 0}, 2}, {{21, 41, 0}, 2}, {{25, 28, 0}, 2},
    {{30, 43, 0}, 2}, {{34, 52, 0}, 2}, {{39, 50, 0}, 2},
};

constexpr StepSequenceGenerator::PartPosition kTopCornerPositions[] = {
    kCornerPositions[0], kCornerPositions[1],
    kCornerPositions[2], kCornerPositions[3],
};

constexpr StepSequenceGenerator::PartPosition kTopEdgePositions[] = {
    kEdgePositions[0], kEdgePositions[1],
    kEdgePositions[2], kEdgePositions[3],
};

constexpr const char* kSideRotationMasks[] = {
    "---------RRR------FFF---------------LLL------BBB------",
    "---------FFF------LLL---------------BBB------RRR------",
    "---------LLL------BBB---------------RRR------FFF------",
    "---------BBB------RRR---------------FFF------LLL------",
};

bool containsChar(const char* pattern, uint8_t size, char value) {
  for (uint8_t i = 0; i < size; ++i) {
    if (pattern[i] == value) return true;
  }
  return false;
}

bool setEqualsLikeOriginal(const char* a, const char* b, uint8_t size) {
  // Original arrayEquals() is A.every(v => B.includes(v)), not positional
  // equality. Cube pieces contain unique colors, so this is set equality here.
  for (uint8_t i = 0; i < size; ++i) {
    if (!containsChar(b, size, a[i])) return false;
  }
  return true;
}

int patternIndex(const char* pattern, uint8_t size, char value) {
  for (uint8_t i = 0; i < size; ++i) {
    if (pattern[i] == value) return i;
  }
  return -1;
}

void readPattern(const StepSequenceGenerator::PartPosition& part,
                 const char* facelets, char out[3]) {
  for (uint8_t i = 0; i < part.size; ++i) out[i] = facelets[part.position[i]];
}

bool maskMatches(const char* facelets, const char* mask) {
  for (uint8_t i = 0; i < 54; ++i) {
    if (mask[i] != '-' && mask[i] != facelets[i]) return false;
  }
  return true;
}

}  // namespace

bool StepSequenceGenerator::generate(uint8_t stepNumber, char out[55],
                                     Stream* log) {
  if (!out || stepNumber < 2 || stepNumber > 7 || !solver_.ready()) {
    if (log) log->println("Step generate failed: invalid step or solver not ready");
    return false;
  }

  const uint32_t started = millis();
  uint32_t attempts = 0;
  char candidate[55]{};

  do {
    ++attempts;
    if (!RandomCubeGenerator::generate(candidate)) return false;
    if (!tradeParts(candidate, stepNumber)) continue;

    if (isStepFacelets(candidate, stepNumber)) {
      std::memcpy(out, candidate, sizeof(candidate));
      if (log) {
        log->printf("Step %u facelets generated in %u ms (%u attempts)\n",
                    static_cast<unsigned>(stepNumber),
                    static_cast<unsigned>(millis() - started),
                    static_cast<unsigned>(attempts));
        log->flush();
      }
      return true;
    }
  } while ((millis() - started) <= kGenerateTimeoutMs);

  if (log) {
    log->printf("Step %u generate timeout after %u attempts\n",
                static_cast<unsigned>(stepNumber),
                static_cast<unsigned>(attempts));
    log->flush();
  }
  return false;
}

bool StepSequenceGenerator::tradeParts(char facelets[55],
                                       uint8_t stepNumber) const {
  if (stepNumber < 5) {
    if (!tradePartList(facelets, kStepTemplates[stepNumber], kEdgePositions,
                       sizeof(kEdgePositions) / sizeof(kEdgePositions[0]), false)) {
      return false;
    }
    return tradePartList(facelets, kStepTemplates[stepNumber], kCornerPositions,
                         sizeof(kCornerPositions) / sizeof(kCornerPositions[0]),
                         false);
  }

  if (!tradePartList(facelets, kStepTemplates[4], kEdgePositions,
                     sizeof(kEdgePositions) / sizeof(kEdgePositions[0]), false)) {
    return false;
  }
  if (!tradePartList(facelets, kStepTemplates[4], kCornerPositions,
                     sizeof(kCornerPositions) / sizeof(kCornerPositions[0]),
                     false)) {
    return false;
  }
  if (!tradePartList(facelets, kStepTemplates[stepNumber], kTopEdgePositions,
                     sizeof(kTopEdgePositions) / sizeof(kTopEdgePositions[0]),
                     true)) {
    return false;
  }
  return tradePartList(facelets, kStepTemplates[stepNumber], kTopCornerPositions,
                       sizeof(kTopCornerPositions) /
                           sizeof(kTopCornerPositions[0]),
                       true);
}

bool StepSequenceGenerator::tradePartList(
    char facelets[55], const char* stepTemplate, const PartPosition* parts,
    size_t partCount, bool lastLayer) const {
  size_t sourceStart = 0;

  for (size_t destinationIndex = 0; destinationIndex < partCount;
       ++destinationIndex) {
    const PartPosition& destination = parts[destinationIndex];
    bool selected = false;
    for (uint8_t i = 0; i < destination.size; ++i) {
      if (stepTemplate[destination.position[i]] != '-') {
        selected = true;
        break;
      }
    }
    if (!selected) continue;

    char targetPattern[3]{};
    char destinationPattern[3]{};
    for (uint8_t i = 0; i < destination.size; ++i) {
      targetPattern[i] = stepTemplate[destination.position[i]];
      destinationPattern[i] = facelets[destination.position[i]];
    }

    size_t sourceIndex = partCount;
    char sourceRawPattern[3]{};

    if (!lastLayer) {
      for (size_t i = 0; i < partCount; ++i) {
        char pattern[3]{};
        readPattern(parts[i], facelets, pattern);
        if (setEqualsLikeOriginal(pattern, targetPattern, destination.size)) {
          sourceIndex = i;
          std::memcpy(sourceRawPattern, pattern, sizeof(sourceRawPattern));
          break;
        }
      }
    } else {
      for (size_t i = sourceStart; i < partCount; ++i) {
        char pattern[3]{};
        readPattern(parts[i], facelets, pattern);
        bool matches = true;
        for (uint8_t j = 0; j < destination.size; ++j) {
          if (targetPattern[j] != '-' &&
              !containsChar(pattern, destination.size, targetPattern[j])) {
            matches = false;
            break;
          }
        }
        if (matches) {
          sourceIndex = i;
          std::memcpy(sourceRawPattern, pattern, sizeof(sourceRawPattern));
          break;
        }
      }
      // Mirrors the original partsPositions.shift() call after each selected
      // last-layer destination, regardless of which source index was found.
      if (sourceStart < partCount) ++sourceStart;
    }

    if (sourceIndex >= partCount) return false;

    uint8_t sourcePosition[3]{};
    if (!lastLayer) {
      for (uint8_t i = 0; i < destination.size; ++i) {
        const int rawIndex =
            patternIndex(sourceRawPattern, destination.size, targetPattern[i]);
        if (rawIndex < 0) return false;
        sourcePosition[i] = parts[sourceIndex].position[rawIndex];
      }
    } else {
      uint8_t order[3] = {0, 1, 2};
      const bool targetContainsDash =
          containsChar(targetPattern, destination.size, '-');
      const int factor = targetContainsDash ? -1 : 1;

      // Reproduce the original JS comparator used before mapping the source
      // sticker order back to positions.
      for (uint8_t i = 0; i < destination.size; ++i) {
        for (uint8_t j = static_cast<uint8_t>(i + 1);
             j < destination.size; ++j) {
          const int ai = patternIndex(targetPattern, destination.size,
                                      sourceRawPattern[order[i]]);
          const int bi = patternIndex(targetPattern, destination.size,
                                      sourceRawPattern[order[j]]);
          const int compare = factor * (ai > bi ? 1 : -1);
          if (compare > 0) std::swap(order[i], order[j]);
        }
      }
      for (uint8_t i = 0; i < destination.size; ++i) {
        sourcePosition[i] = parts[sourceIndex].position[order[i]];
      }
    }

    char sourcePattern[3]{};
    for (uint8_t i = 0; i < destination.size; ++i) {
      sourcePattern[i] = facelets[sourcePosition[i]];
    }

    for (uint8_t i = 0; i < destination.size; ++i) {
      facelets[sourcePosition[i]] = destinationPattern[i];
    }
    for (uint8_t i = 0; i < destination.size; ++i) {
      facelets[destination.position[i]] = sourcePattern[i];
    }
  }

  return true;
}

bool StepSequenceGenerator::isNextStepPattern(const char* facelets,
                                              uint8_t stepNumber) const {
  if (maskMatches(facelets, kStepTemplates[stepNumber + 1])) return true;
  for (const char* mask : kSideRotationMasks) {
    if (maskMatches(facelets, mask)) return true;
  }
  return false;
}

bool StepSequenceGenerator::isStepFacelets(const char* facelets,
                                           uint8_t stepNumber) const {
  return solver_.verify(facelets) == 0 &&
         !isNextStepPattern(facelets, stepNumber);
}

}  // namespace sequence
