#pragma once

#include <Arduino.h>

#include "StepSequenceGenerator.h"

namespace solver {
class Min2PhaseSolver;
}

namespace sequence {

struct GeneratedSequenceResult {
  bool success = false;
  int error = 0;
  uint8_t solutionLength = 0;
  uint32_t generationMs = 0;
  uint32_t solveMs = 0;
  char facelets[55]{};
  char sequence[128]{};
};

class SequenceGenerator {
 public:
  explicit SequenceGenerator(solver::Min2PhaseSolver& solver)
      : solver_(solver), stepGenerator_(solver) {}

  bool random(GeneratedSequenceResult& out, Stream* log = nullptr);
  bool step(uint8_t stepNumber, GeneratedSequenceResult& out,
            Stream* log = nullptr);
  bool scrambleForFacelets(const char* facelets, GeneratedSequenceResult& out,
                           Stream* log = nullptr);

  bool selfTest(Stream& out);

 private:
  static bool reverseSolution(const char* solution, char* out, size_t outSize);

  solver::Min2PhaseSolver& solver_;
  StepSequenceGenerator stepGenerator_;
};

}  // namespace sequence
