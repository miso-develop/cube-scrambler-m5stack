#pragma once

#include <Arduino.h>

#include "SolverTableStorage.h"

namespace solver {

constexpr size_t kMin2PhaseTableSectionCount = 32;
constexpr size_t kMin2PhaseTableDataBytes = 1003678;

// Validates that a generic, CRC-checked SolverTableStorage image contains the
// exact fixed-width sections expected by the NanoC6 Min2Phase port.
// The table payload remains Flash-backed; this function does not copy it to RAM.
bool validateMin2PhaseTableCatalog(const SolverTableStorage& storage, Stream& out);

}  // namespace solver
