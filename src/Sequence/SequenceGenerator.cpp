#include "SequenceGenerator.h"

#include <cstring>

#include "RandomCubeGenerator.h"
#include "Solver/Min2PhaseSolver.h"

namespace sequence {

bool SequenceGenerator::reverseSolution(const char* solution, char* out,
                                        size_t outSize) {
  if (!solution || !out || outSize == 0) return false;
  out[0] = '\0';

  char copy[128]{};
  if (std::strlen(solution) >= sizeof(copy)) return false;
  std::strcpy(copy, solution);

  char* tokens[32]{};
  size_t tokenCount = 0;
  char* save = nullptr;
  for (char* token = ::strtok_r(copy, " ", &save); token != nullptr;
       token = ::strtok_r(nullptr, " ", &save)) {
    if (tokenCount >= sizeof(tokens) / sizeof(tokens[0])) return false;
    tokens[tokenCount++] = token;
  }

  size_t used = 0;
  for (size_t reverseIndex = 0; reverseIndex < tokenCount; ++reverseIndex) {
    const char* token = tokens[tokenCount - 1 - reverseIndex];
    const size_t length = std::strlen(token);
    if (length < 1 || length > 2) return false;

    char inverse[4]{};
    inverse[0] = token[0];
    if (length == 2 && token[1] == '2') {
      inverse[1] = '2';
    } else if (length == 2 && token[1] == '\'') {
      inverse[1] = '\0';
    } else if (length == 1) {
      inverse[1] = '\'';
      inverse[2] = '\0';
    } else {
      return false;
    }

    const size_t inverseLength = std::strlen(inverse);
    const size_t required = inverseLength + (reverseIndex > 0 ? 1 : 0);
    if (used + required + 1 > outSize) return false;

    if (reverseIndex > 0) out[used++] = ' ';
    std::memcpy(out + used, inverse, inverseLength);
    used += inverseLength;
    out[used] = '\0';
  }

  return true;
}

bool SequenceGenerator::scrambleForFacelets(const char* facelets,
                                            GeneratedSequenceResult& out,
                                            Stream* log) {
  out = GeneratedSequenceResult{};
  if (!facelets || !solver_.ready()) {
    out.error = -100;
    return false;
  }

  const int verifyResult = solver_.verify(facelets);
  if (verifyResult != 0) {
    out.error = verifyResult;
    if (log) log->printf("Sequence generate failed: invalid facelets (%d)\n",
                         verifyResult);
    return false;
  }

  std::strncpy(out.facelets, facelets, sizeof(out.facelets) - 1);
  const solver::Min2PhaseSolveResult solved = solver_.solve(facelets, 21);
  out.solveMs = solved.elapsedMs;
  out.solutionLength = solved.length;
  if (!solved.success) {
    out.error = solved.error;
    if (log) {
      log->printf("Sequence generate failed: solve error=%d time=%u ms\n",
                  solved.error, static_cast<unsigned>(solved.elapsedMs));
      log->flush();
    }
    return false;
  }

  if (!reverseSolution(solved.moves, out.sequence, sizeof(out.sequence))) {
    out.error = -101;
    if (log) log->println("Sequence generate failed: could not reverse solution");
    return false;
  }

  out.success = true;
  return true;
}

bool SequenceGenerator::random(GeneratedSequenceResult& out, Stream* log) {
  const uint32_t started = millis();
  char facelets[55]{};
  if (!RandomCubeGenerator::generate(facelets)) {
    out = GeneratedSequenceResult{};
    out.error = -102;
    return false;
  }

  if (!scrambleForFacelets(facelets, out, log)) return false;
  out.generationMs = millis() - started;
  if (log) {
    log->printf("Random scramble generated in %u ms (solve=%u ms, length=%u)\n",
                static_cast<unsigned>(out.generationMs),
                static_cast<unsigned>(out.solveMs),
                static_cast<unsigned>(out.solutionLength));
    log->flush();
  }
  return true;
}

bool SequenceGenerator::step(uint8_t stepNumber, GeneratedSequenceResult& out,
                             Stream* log) {
  const uint32_t started = millis();
  char facelets[55]{};
  if (!stepGenerator_.generate(stepNumber, facelets, log)) {
    out = GeneratedSequenceResult{};
    out.error = -103;
    return false;
  }

  if (!scrambleForFacelets(facelets, out, log)) return false;
  out.generationMs = millis() - started;
  if (log) {
    log->printf("Step %u scramble generated in %u ms (solve=%u ms, length=%u)\n",
                static_cast<unsigned>(stepNumber),
                static_cast<unsigned>(out.generationMs),
                static_cast<unsigned>(out.solveMs),
                static_cast<unsigned>(out.solutionLength));
    log->flush();
  }
  return true;
}

bool SequenceGenerator::selfTest(Stream& out) {
  out.println();
  out.println("SequenceGenerator self-test");
  bool passed = true;

  char reversed[32]{};
  const bool reversePassed =
      reverseSolution("R U2 F'", reversed, sizeof(reversed)) &&
      std::strcmp(reversed, "F U2 R'") == 0;
  out.printf("Reverse solution: %s (%s)\n", reversePassed ? "PASS" : "FAIL",
             reversed);
  passed &= reversePassed;

  GeneratedSequenceResult randomResult{};
  const bool randomPassed = random(randomResult, nullptr) &&
                            solver_.verify(randomResult.facelets) == 0 &&
                            randomResult.sequence[0] != '\0';
  out.printf("Random scramble: %s generation=%u ms solve=%u ms length=%u\n",
             randomPassed ? "PASS" : "FAIL",
             static_cast<unsigned>(randomResult.generationMs),
             static_cast<unsigned>(randomResult.solveMs),
             static_cast<unsigned>(randomResult.solutionLength));
  if (randomPassed) {
    out.printf("Random facelets: %s\n", randomResult.facelets);
    out.printf("Random sequence: %s\n", randomResult.sequence);
  }
  passed &= randomPassed;

  for (uint8_t stepNumber = 2; stepNumber <= 7; ++stepNumber) {
    GeneratedSequenceResult stepResult{};
    const bool stepPassed = step(stepNumber, stepResult, nullptr) &&
                            solver_.verify(stepResult.facelets) == 0 &&
                            stepResult.sequence[0] != '\0';
    out.printf("Step %u: %s generation=%u ms solve=%u ms length=%u\n",
               static_cast<unsigned>(stepNumber),
               stepPassed ? "PASS" : "FAIL",
               static_cast<unsigned>(stepResult.generationMs),
               static_cast<unsigned>(stepResult.solveMs),
               static_cast<unsigned>(stepResult.solutionLength));
    if (!stepPassed) passed = false;
  }

  out.printf("SequenceGenerator self-test: %s\n", passed ? "PASS" : "FAIL");
  out.flush();
  return passed;
}

}  // namespace sequence
