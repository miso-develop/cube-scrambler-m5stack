#pragma once

#include <Arduino.h>

namespace solver {

// Prints the Phase 2 baseline used before integrating a Min2Phase backend.
// This intentionally allocates no solver tables in RAM.
void printFeasibilityBaseline(Stream& out);

// Modeled size of the major immutable Min2Phase coordinate/pruning tables.
size_t majorTableFootprintBytes();

}  // namespace solver
