#pragma once

#include <Arduino.h>

#include "SolverTableStorage.h"

namespace solver {

struct Min2PhaseSolveResult {
  bool success = false;
  int error = 0;
  uint8_t length = 0;
  uint32_t elapsedMs = 0;
  char moves[128] = {};
};

// Flash-backed, no-runtime-table-generation Min2Phase solver for NanoC6.
//
// The large move/pruning tables are bound directly to SolverTableStorage mmap
// sections. Only the search state, move cubes and recursion path live in SRAM.
class Min2PhaseSolver {
 public:
  Min2PhaseSolver() = default;

  bool begin(const SolverTableStorage& storage, Stream& out);
  bool ready() const { return ready_; }

  // Returns 0 for a valid cube, otherwise the Min2Phase-compatible validation
  // code -1 .. -6.
  int verify(const char* facelets) const;

  Min2PhaseSolveResult solve(const char* facelets, uint8_t maxDepth = 21);

  // Phase 2 integration smoke test. Exercises facelet conversion/parsing,
  // Flash-backed table lookup, phase-1 search and phase-2 search.
  bool selfTest(Stream& out);

 private:
  struct Tables;
  struct CubieCube;
  struct CoordCube;

  bool bindTables(const SolverTableStorage& storage, Stream& out);
  void initMoveCubes();
  void initPhase2MoveMasks();

  int verifyCube(CubieCube& cube, const char* facelets) const;
  bool parseScramble(const char* sequence, CubieCube& cube) const;
  bool parseMoves(const char* sequence, CubieCube& cube) const;
  void applyMove(const CubieCube& source, uint8_t move, CubieCube& target) const;
  bool isSolved(const CubieCube& cube) const;
  void toFacelets(const CubieCube& cube, char out[55]) const;

  CoordCube makeCoord(const CubieCube& cube) const;
  CoordCube moveCoord(const CoordCube& source, uint8_t move) const;
  uint8_t phase1Pruning(const CoordCube& coord) const;
  uint8_t pruning(const uint32_t* table, uint32_t index) const;

  bool searchPhase1(uint8_t remaining, int lastAxis);
  bool tryPhase2(uint8_t phase1Length);
  bool searchPhase2(uint16_t edge, uint8_t esym,
                    uint16_t corn, uint8_t csym,
                    uint8_t mid, uint8_t remaining,
                    uint8_t depth, uint8_t lastMove);

  uint16_t getPermSymInv(uint16_t index, uint8_t sym, bool corner) const;
  uint16_t eSymToCSym(uint16_t index) const;
  void formatSolution(uint8_t length, Min2PhaseSolveResult& result) const;

  Tables* tables_ = nullptr;
  bool ready_ = false;

  CubieCube* moveCubes_ = nullptr;
  CubieCube* cubePath_ = nullptr;
  CoordCube* coordPath_ = nullptr;
  uint8_t* moves_ = nullptr;
  uint16_t phase2MoveMask_[11] = {};

  uint8_t phase1Length_ = 0;
  uint8_t maxDepth_ = 21;
  uint8_t solutionLength_ = 0;
};

}  // namespace solver
