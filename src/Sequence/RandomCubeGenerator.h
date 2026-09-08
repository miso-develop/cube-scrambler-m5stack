#pragma once

#include <Arduino.h>

namespace sequence {

// Generates a uniformly distributed legal 3x3 cubie state using the same
// coordinate ranges/parity rule as min2phase.randomCube(), then converts it to
// URFDLB facelets.
class RandomCubeGenerator {
 public:
  static bool generate(char out[55]);

 private:
  static uint32_t randomBelow(uint32_t upperExclusive);
};

}  // namespace sequence
