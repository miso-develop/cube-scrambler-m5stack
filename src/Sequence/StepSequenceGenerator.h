#pragma once

#include <Arduino.h>

namespace solver {
class Min2PhaseSolver;
}

namespace sequence {

class StepSequenceGenerator {
 public:
  struct PartPosition {
    uint8_t position[3];
    uint8_t size;
  };

  explicit StepSequenceGenerator(solver::Min2PhaseSolver& solver)
      : solver_(solver) {}

  // Generates a legal target cube state for CFOP-like steps 2..7 using the
  // same templates and part-trading algorithm as the original TypeScript app.
  bool generate(uint8_t stepNumber, char out[55], Stream* log = nullptr);

 private:
  bool tradeParts(char facelets[55], uint8_t stepNumber) const;
  bool tradePartList(char facelets[55], const char* stepTemplate,
                     const PartPosition* parts, size_t partCount,
                     bool lastLayer) const;
  bool isStepFacelets(const char* facelets, uint8_t stepNumber) const;
  bool isNextStepPattern(const char* facelets, uint8_t stepNumber) const;

  solver::Min2PhaseSolver& solver_;
};

}  // namespace sequence
