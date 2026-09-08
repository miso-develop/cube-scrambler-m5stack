#include "Min2PhaseSolver.h"

#include <algorithm>
#include <cstring>

#include "SolverTableCatalog.h"

// Algorithm/reference source:
//   cs0x7f/min2phase.js@0ba83a6177d816f72af1a45c9015349da597456a
// Selected upstream license: MIT. See docs/THIRD_PARTY.md.
// Copyright (c) 2023 Chen Shuang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

namespace solver {
namespace {

constexpr uint8_t kNMoves = 18;
constexpr uint8_t kNMoves2 = 10;
constexpr uint16_t kNSlice = 495;
constexpr uint16_t kNComb = 140;
constexpr uint8_t kNMPerm = 24;
constexpr uint8_t kMaxPhase1Depth = 12;
constexpr uint8_t kMaxPhase2Depth = 13;
constexpr uint32_t kSymE2CMagic = 0x00DDDD00u;

constexpr uint8_t kUd2Std[kNMoves] = {
    0, 1, 2, 4, 7, 9, 10, 11, 13, 16, 3, 5, 6, 8, 12, 14, 15, 17};

constexpr const char* kMoveNames[kNMoves] = {
    "U", "U2", "U'", "R", "R2", "R'", "F", "F2", "F'",
    "D", "D2", "D'", "L", "L2", "L'", "B", "B2", "B'"};

constexpr uint8_t kCornerFacelet[8][3] = {
    {8, 9, 20}, {6, 18, 38}, {0, 36, 47}, {2, 45, 11},
    {29, 26, 15}, {27, 44, 24}, {33, 53, 42}, {35, 17, 51}};

constexpr uint8_t kEdgeFacelet[12][2] = {
    {5, 10}, {7, 19}, {3, 37}, {1, 46}, {32, 16}, {28, 25},
    {30, 43}, {34, 52}, {23, 12}, {21, 41}, {50, 39}, {48, 14}};

constexpr uint32_t kFactorial[13] = {
    1u, 1u, 2u, 6u, 24u, 120u, 720u,
    5040u, 40320u, 362880u, 3628800u, 39916800u, 479001600u};

uint16_t choose(uint8_t n, uint8_t k) {
  if (k > n) return 0;
  if (k == 0 || k == n) return 1;
  if (k > n - k) k = n - k;
  uint32_t value = 1;
  for (uint8_t i = 1; i <= k; ++i) {
    value = value * (n - k + i) / i;
  }
  return static_cast<uint16_t>(value);
}

void setNPerm(uint8_t* arr, uint32_t idx, uint8_t n) {
  --n;
  uint32_t val = 0x76543210u;
  for (uint8_t i = 0; i < n; ++i) {
    const uint32_t p = kFactorial[n - i];
    uint32_t v = idx / p;
    idx %= p;
    v <<= 2;
    arr[i] = static_cast<uint8_t>((arr[i] & 0xf0u) | ((val >> v) & 0xfu));
    const uint32_t mask = (1u << v) - 1u;
    val = (val & mask) + ((val >> 4) & ~mask);
  }
  arr[n] = static_cast<uint8_t>((arr[n] & 0xf0u) | (val & 0xfu));
}

uint32_t getNPerm(const uint8_t* arr, uint8_t n) {
  uint32_t idx = 0;
  uint32_t val = 0x76543210u;
  for (uint8_t i = 0; i < n - 1; ++i) {
    const uint8_t v = static_cast<uint8_t>((arr[i] & 0xfu) << 2);
    idx = (n - i) * idx + ((val >> v) & 0xfu);
    val -= 0x11111110u << v;
  }
  return idx;
}

void setNPermFull(uint8_t* arr, uint32_t idx, uint8_t n) {
  arr[n - 1] &= 0xf0u;
  for (int i = n - 2; i >= 0; --i) {
    arr[i] = static_cast<uint8_t>((arr[i] & 0xf0u) | (idx % (n - i)));
    idx /= (n - i);
    for (uint8_t j = static_cast<uint8_t>(i + 1); j < n; ++j) {
      if ((arr[j] & 0xfu) >= (arr[i] & 0xfu)) ++arr[j];
    }
  }
}

uint32_t getNPermFull(const uint8_t* arr, uint8_t n) {
  uint32_t idx = 0;
  for (uint8_t i = 0; i < n; ++i) {
    idx *= n - i;
    for (uint8_t j = i + 1; j < n; ++j) {
      if ((arr[j] & 0xfu) < (arr[i] & 0xfu)) ++idx;
    }
  }
  return idx;
}

uint16_t getComb(const uint8_t* arr, uint8_t length, uint8_t mask) {
  uint16_t idx = 0;
  int r = 4;
  for (int i = length - 1; i >= 0 && r > 0; --i) {
    const uint8_t perm = arr[i] & 0xfu;
    if ((perm & 0xcu) == mask) {
      idx += choose(static_cast<uint8_t>(i), static_cast<uint8_t>(r));
      --r;
    }
  }
  return idx;
}

uint8_t getNParity(uint32_t idx, uint8_t n) {
  uint8_t parity = 0;
  for (int i = n - 2; i >= 0; --i) {
    parity ^= static_cast<uint8_t>(idx % (n - i));
    idx /= (n - i);
  }
  return parity & 1u;
}

int faceIndex(char face) {
  switch (face) {
    case 'U': return 0;
    case 'R': return 1;
    case 'F': return 2;
    case 'D': return 3;
    case 'L': return 4;
    case 'B': return 5;
    default: return -1;
  }
}

}  // namespace

struct Min2PhaseSolver::Tables {
  const uint16_t* twstMove = nullptr;
  const uint16_t* flipMove = nullptr;
  const uint16_t* sliceMove = nullptr;
  const uint16_t* sliceConj = nullptr;
  const uint16_t* cPermMove = nullptr;
  const uint16_t* ePermMove = nullptr;
  const uint8_t* mPermMove = nullptr;
  const uint8_t* mPermConj = nullptr;
  const uint8_t* cCombPConj = nullptr;
  const uint8_t* symMult = nullptr;
  const uint8_t* symMultInv = nullptr;
  const uint8_t* symMoveUD = nullptr;
  const uint8_t* sym8Move = nullptr;
  const uint16_t* flipR2S = nullptr;
  const uint16_t* flipS2RF = nullptr;
  const uint16_t* twstR2S = nullptr;
  const uint16_t* ePermR2S = nullptr;
  const uint8_t* perm2CombP = nullptr;
  const uint16_t* permInvEdgeSym = nullptr;
  const uint32_t* twstFlipPrun = nullptr;
  const uint32_t* sliceTwstPrun = nullptr;
  const uint32_t* sliceFlipPrun = nullptr;
  const uint32_t* mcPermPrun = nullptr;
  const uint32_t* ePermCCombPPrun = nullptr;
};

struct Min2PhaseSolver::CubieCube {
  uint8_t ca[8];
  uint8_t ea[12];

  CubieCube() { reset(); }

  void reset() {
    for (uint8_t i = 0; i < 8; ++i) ca[i] = i;
    for (uint8_t i = 0; i < 12; ++i) ea[i] = i;
  }

  void initCoord(uint32_t cperm, uint16_t twst, uint32_t eperm, uint16_t flip) {
    reset();
    setNPerm(ca, cperm, 8);
    setTwst(twst);
    setNPermFull(ea, eperm, 12);
    setFlip(flip);
  }

  void setFlip(uint16_t idx) {
    uint8_t parity = 0;
    for (int i = 10; i >= 0; --i, idx >>= 1) {
      ea[i] = static_cast<uint8_t>((ea[i] & 0xfu) | ((idx & 1u) << 4));
      parity ^= ea[i];
    }
    ea[11] = static_cast<uint8_t>((ea[11] & 0xfu) | (parity & 0x10u));
  }

  uint16_t getFlip() const {
    uint16_t idx = 0;
    for (uint8_t i = 0; i < 11; ++i) {
      idx = static_cast<uint16_t>((idx << 1) | ((ea[i] >> 4) & 1u));
    }
    return idx;
  }

  void setTwst(uint16_t idx) {
    int twst = 15;
    for (int i = 6; i >= 0; --i, idx /= 3) {
      const uint8_t ori = idx % 3;
      ca[i] = static_cast<uint8_t>((ca[i] & 0xfu) | (ori << 4));
      twst -= ori;
    }
    ca[7] = static_cast<uint8_t>((ca[7] & 0xfu) | ((twst % 3) << 4));
  }

  uint16_t getTwst() const {
    uint16_t idx = 0;
    for (uint8_t i = 0; i < 7; ++i) {
      idx = static_cast<uint16_t>(3 * idx + (ca[i] >> 4));
    }
    return idx;
  }

  uint16_t getSlice() const {
    return static_cast<uint16_t>(494 - getComb(ea, 12, 8));
  }

  uint8_t getMPerm() const {
    return static_cast<uint8_t>(getNPermFull(ea, 12) % 24);
  }

  uint32_t getCPerm() const { return getNPerm(ca, 8); }
  uint32_t getEPerm() const { return getNPerm(ea, 8); }

  static void edgeMult(const CubieCube& a, const CubieCube& b, CubieCube& out) {
    for (uint8_t edge = 0; edge < 12; ++edge) {
      out.ea[edge] = static_cast<uint8_t>(
          a.ea[b.ea[edge] & 0xfu] ^ (b.ea[edge] & 0x10u));
    }
  }

  static void cornMult(const CubieCube& a, const CubieCube& b, CubieCube& out) {
    for (uint8_t corn = 0; corn < 8; ++corn) {
      const uint8_t ori = static_cast<uint8_t>(
          ((a.ca[b.ca[corn] & 0xfu] >> 4) + (b.ca[corn] >> 4)) % 3);
      out.ca[corn] = static_cast<uint8_t>(
          (a.ca[b.ca[corn] & 0xfu] & 0xfu) | (ori << 4));
    }
  }

  int fromFacelets(const char* facelets) {
    if (!facelets || std::strlen(facelets) < 54) return -1;

    const char centers[6] = {
        facelets[4], facelets[13], facelets[22],
        facelets[31], facelets[40], facelets[49]};
    uint8_t f[54];
    uint32_t count = 0;

    for (uint8_t i = 0; i < 54; ++i) {
      int color = -1;
      for (uint8_t c = 0; c < 6; ++c) {
        if (facelets[i] == centers[c]) {
          color = c;
          break;
        }
      }
      if (color < 0) return -1;
      f[i] = static_cast<uint8_t>(color);
      count += 1u << (f[i] << 2);
    }
    if (count != 0x999999u) return -1;

    for (uint8_t i = 0; i < 8; ++i) {
      uint8_t ori = 0;
      for (; ori < 3; ++ori) {
        const uint8_t color = f[kCornerFacelet[i][ori]];
        if (color == 0 || color == 3) break;
      }
      if (ori == 3) return -1;
      const uint8_t col1 = f[kCornerFacelet[i][(ori + 1) % 3]];
      const uint8_t col2 = f[kCornerFacelet[i][(ori + 2) % 3]];
      bool found = false;
      for (uint8_t j = 0; j < 8; ++j) {
        if (col1 == kCornerFacelet[j][1] / 9 &&
            col2 == kCornerFacelet[j][2] / 9) {
          ca[i] = static_cast<uint8_t>(j | ((ori % 3) << 4));
          found = true;
          break;
        }
      }
      if (!found) return -1;
    }

    for (uint8_t i = 0; i < 12; ++i) {
      bool found = false;
      for (uint8_t j = 0; j < 12; ++j) {
        if (f[kEdgeFacelet[i][0]] == kEdgeFacelet[j][0] / 9 &&
            f[kEdgeFacelet[i][1]] == kEdgeFacelet[j][1] / 9) {
          ea[i] = j;
          found = true;
          break;
        }
        if (f[kEdgeFacelet[i][0]] == kEdgeFacelet[j][1] / 9 &&
            f[kEdgeFacelet[i][1]] == kEdgeFacelet[j][0] / 9) {
          ea[i] = static_cast<uint8_t>(j | 0x10u);
          found = true;
          break;
        }
      }
      if (!found) return -1;
    }
    return 0;
  }
};

struct Min2PhaseSolver::CoordCube {
  uint16_t twst = 0;
  uint16_t flip = 0;
  uint16_t slice = 0;
  uint8_t prun = 0;
};

bool Min2PhaseSolver::begin(const SolverTableStorage& storage, Stream& out) {
  ready_ = false;
  if (!validateMin2PhaseTableCatalog(storage, out)) return false;

  static Tables tables;
  static CubieCube moveCubes[kNMoves];
  static CubieCube cubePath[24];
  static CoordCube coordPath[24];
  static uint8_t moves[32];

  tables_ = &tables;
  moveCubes_ = moveCubes;
  cubePath_ = cubePath;
  coordPath_ = coordPath;
  moves_ = moves;

  if (!bindTables(storage, out)) return false;
  initMoveCubes();
  initPhase2MoveMasks();
  ready_ = true;
  out.println("Min2Phase solver core: READY");
  return true;
}

bool Min2PhaseSolver::bindTables(const SolverTableStorage& storage, Stream& out) {
  auto bind = [&](const char* name, uint32_t elementSize, const uint8_t*& data) {
    SolverTableSectionView view;
    if (!storage.findSection(name, view) || view.elementSize != elementSize) {
      out.printf("Min2Phase solver bind: FAILED (%s)\n", name);
      return false;
    }
    data = view.data;
    return true;
  };

#define BIND_TABLE(field, name, type, elementSize)                                \
  do {                                                                            \
    const uint8_t* raw = nullptr;                                                  \
    if (!bind(name, elementSize, raw)) return false;                               \
    tables_->field = reinterpret_cast<const type*>(raw);                           \
  } while (0)

  BIND_TABLE(twstMove, "TwstMove", uint16_t, 2);
  BIND_TABLE(flipMove, "FlipMove", uint16_t, 2);
  BIND_TABLE(sliceMove, "SliceMove", uint16_t, 2);
  BIND_TABLE(sliceConj, "SliceConj", uint16_t, 2);
  BIND_TABLE(cPermMove, "CPermMove", uint16_t, 2);
  BIND_TABLE(ePermMove, "EPermMove", uint16_t, 2);
  BIND_TABLE(mPermMove, "MPermMove", uint8_t, 1);
  BIND_TABLE(mPermConj, "MPermConj", uint8_t, 1);
  BIND_TABLE(cCombPConj, "CCombPConj", uint8_t, 1);
  BIND_TABLE(symMult, "SymMult", uint8_t, 1);
  BIND_TABLE(symMultInv, "SymMultInv", uint8_t, 1);
  BIND_TABLE(symMoveUD, "SymMoveUD", uint8_t, 1);
  BIND_TABLE(sym8Move, "Sym8Move", uint8_t, 1);
  BIND_TABLE(flipR2S, "FlipR2S", uint16_t, 2);
  BIND_TABLE(flipS2RF, "FlipS2RF", uint16_t, 2);
  BIND_TABLE(twstR2S, "TwstR2S", uint16_t, 2);
  BIND_TABLE(ePermR2S, "EPermR2S", uint16_t, 2);
  BIND_TABLE(perm2CombP, "Perm2CombP", uint8_t, 1);
  BIND_TABLE(permInvEdgeSym, "PermInvEdgeSym", uint16_t, 2);
  BIND_TABLE(twstFlipPrun, "TwstFlipPrun", uint32_t, 4);
  BIND_TABLE(sliceTwstPrun, "SliceTwstPrun", uint32_t, 4);
  BIND_TABLE(sliceFlipPrun, "SliceFlipPrun", uint32_t, 4);
  BIND_TABLE(mcPermPrun, "MCPermPrun", uint32_t, 4);
  BIND_TABLE(ePermCCombPPrun, "EPermCCombPPrun", uint32_t, 4);

#undef BIND_TABLE
  return true;
}

void Min2PhaseSolver::initMoveCubes() {
  for (uint8_t i = 0; i < kNMoves; ++i) moveCubes_[i].reset();

  moveCubes_[0].initCoord(15120, 0, 119750400, 0);
  moveCubes_[3].initCoord(21021, 1494, 323403417, 0);
  moveCubes_[6].initCoord(8064, 1236, 29441808, 550);
  moveCubes_[9].initCoord(9, 0, 5880, 0);
  moveCubes_[12].initCoord(1230, 412, 2949660, 0);
  moveCubes_[15].initCoord(224, 137, 328552, 137);

  for (uint8_t axis = 0; axis < kNMoves; axis += 3) {
    for (uint8_t power = 0; power < 2; ++power) {
      CubieCube::edgeMult(moveCubes_[axis + power], moveCubes_[axis],
                          moveCubes_[axis + power + 1]);
      CubieCube::cornMult(moveCubes_[axis + power], moveCubes_[axis],
                          moveCubes_[axis + power + 1]);
    }
  }
}

void Min2PhaseSolver::initPhase2MoveMasks() {
  for (uint8_t i = 0; i < 10; ++i) {
    const uint8_t ix = kUd2Std[i] / 3;
    uint16_t mask = 0;
    for (uint8_t j = 0; j < 10; ++j) {
      const uint8_t jx = kUd2Std[j] / 3;
      if (ix == jx || ((ix % 3) == (jx % 3) && ix >= jx)) {
        mask |= static_cast<uint16_t>(1u << j);
      }
    }
    phase2MoveMask_[i] = mask;
  }
  phase2MoveMask_[10] = 0;
}

int Min2PhaseSolver::verifyCube(CubieCube& cube, const char* facelets) const {
  if (cube.fromFacelets(facelets) != 0) return -1;

  uint8_t sum = 0;
  uint16_t edgeMask = 0;
  for (uint8_t e = 0; e < 12; ++e) {
    edgeMask |= static_cast<uint16_t>(1u << (cube.ea[e] & 0xfu));
    sum ^= cube.ea[e] >> 4;
  }
  if (edgeMask != 0xfffu) return -2;
  if (sum != 0) return -3;

  uint16_t cornerMask = 0;
  sum = 0;
  for (uint8_t c = 0; c < 8; ++c) {
    cornerMask |= static_cast<uint16_t>(1u << (cube.ca[c] & 0xfu));
    sum = static_cast<uint8_t>(sum + (cube.ca[c] >> 4));
  }
  if (cornerMask != 0xffu) return -4;
  if ((sum % 3) != 0) return -5;

  if ((getNParity(getNPermFull(cube.ea, 12), 12) ^
       getNParity(cube.getCPerm(), 8)) != 0) {
    return -6;
  }
  return 0;
}

int Min2PhaseSolver::verify(const char* facelets) const {
  CubieCube cube;
  return verifyCube(cube, facelets);
}

uint8_t Min2PhaseSolver::pruning(const uint32_t* table, uint32_t index) const {
  return static_cast<uint8_t>((table[index >> 3] >> ((index & 7u) << 2)) & 0xfu);
}

uint8_t Min2PhaseSolver::phase1Pruning(const CoordCube& coord) const {
  const uint32_t twstSlice =
      (coord.twst >> 3) * kNSlice +
      tables_->sliceConj[coord.slice * 8 + (coord.twst & 7u)];
  const uint32_t flipSlice =
      (coord.flip >> 3) * kNSlice +
      tables_->sliceConj[coord.slice * 8 + (coord.flip & 7u)];
  const uint32_t twstFlip =
      (static_cast<uint32_t>(coord.twst >> 3) << 11) |
      tables_->flipS2RF[coord.flip ^ (coord.twst & 7u)];

  uint8_t result = pruning(tables_->sliceTwstPrun, twstSlice);
  result = std::max(result, pruning(tables_->sliceFlipPrun, flipSlice));
  result = std::max(result, pruning(tables_->twstFlipPrun, twstFlip));
  return result;
}

Min2PhaseSolver::CoordCube Min2PhaseSolver::makeCoord(const CubieCube& cube) const {
  CoordCube coord;
  coord.twst = tables_->twstR2S[cube.getTwst()];
  coord.flip = tables_->flipR2S[cube.getFlip()];
  coord.slice = cube.getSlice();
  coord.prun = phase1Pruning(coord);
  return coord;
}

Min2PhaseSolver::CoordCube Min2PhaseSolver::moveCoord(
    const CoordCube& source, uint8_t move) const {
  CoordCube target;
  target.slice = tables_->sliceMove[source.slice * kNMoves + move];

  const uint8_t flipSym = source.flip & 7u;
  const uint8_t flipMove = tables_->sym8Move[move * 8 + flipSym];
  target.flip = static_cast<uint16_t>(
      tables_->flipMove[(source.flip >> 3) * kNMoves + flipMove] ^ flipSym);

  const uint8_t twstSym = source.twst & 7u;
  const uint8_t twstMove = tables_->sym8Move[move * 8 + twstSym];
  target.twst = static_cast<uint16_t>(
      tables_->twstMove[(source.twst >> 3) * kNMoves + twstMove] ^ twstSym);

  target.prun = phase1Pruning(target);
  return target;
}

void Min2PhaseSolver::applyMove(const CubieCube& source, uint8_t move,
                                CubieCube& target) const {
  CubieCube::cornMult(source, moveCubes_[move], target);
  CubieCube::edgeMult(source, moveCubes_[move], target);
}

uint16_t Min2PhaseSolver::eSymToCSym(uint16_t index) const {
  return static_cast<uint16_t>(
      index ^ ((kSymE2CMagic >> ((index & 0xfu) << 1)) & 3u));
}

uint16_t Min2PhaseSolver::getPermSymInv(uint16_t index, uint8_t sym,
                                        bool corner) const {
  uint16_t inverse = tables_->permInvEdgeSym[index];
  if (corner) inverse = eSymToCSym(inverse);
  return static_cast<uint16_t>(
      (inverse & 0xfff0u) |
      tables_->symMult[(inverse & 0xfu) * 16 + sym]);
}

bool Min2PhaseSolver::searchPhase1(uint8_t remaining, int lastAxis) {
  const uint8_t depth = phase1Length_ - remaining;
  const CoordCube& node = coordPath_[depth];
  if (node.prun > remaining) return false;

  if (remaining == 0) {
    if (node.prun != 0) return false;
    return tryPhase2(phase1Length_);
  }

  for (int axis = 0; axis < kNMoves; axis += 3) {
    if (axis == lastAxis || axis == lastAxis - 9) continue;

    for (uint8_t power = 0; power < 3; ++power) {
      const uint8_t move = static_cast<uint8_t>(axis + power);
      coordPath_[depth + 1] = moveCoord(node, move);
      if (coordPath_[depth + 1].prun > remaining - 1) continue;

      moves_[depth] = move;
      applyMove(cubePath_[depth], move, cubePath_[depth + 1]);
      if (searchPhase1(remaining - 1, axis)) return true;
    }
  }
  return false;
}

bool Min2PhaseSolver::tryPhase2(uint8_t phase1Length) {
  const CubieCube& cube = cubePath_[phase1Length];

  uint16_t cornPacked = eSymToCSym(tables_->ePermR2S[cube.getCPerm()]);
  uint8_t csym = cornPacked & 0xfu;
  uint16_t corn = cornPacked >> 4;

  uint16_t edgePacked = tables_->ePermR2S[cube.getEPerm()];
  uint8_t esym = edgePacked & 0xfu;
  uint16_t edge = edgePacked >> 4;
  const uint8_t mid = cube.getMPerm();

  const uint8_t comb = tables_->perm2CombP[corn] & 0xffu;
  const uint8_t symRelation = tables_->symMultInv[esym * 16 + csym];
  const uint32_t edgeIndex =
      edge * kNComb + tables_->cCombPConj[comb * 16 + symRelation];
  const uint32_t cornIndex =
      corn * kNMPerm + tables_->mPermConj[mid * 16 + csym];

  uint8_t lower = pruning(tables_->ePermCCombPPrun, edgeIndex);
  lower = std::max(lower, pruning(tables_->mcPermPrun, cornIndex));

  const uint8_t available = maxDepth_ - phase1Length;
  const uint8_t maxPhase2 = std::min<uint8_t>(kMaxPhase2Depth, available);
  if (lower > maxPhase2) return false;

  for (uint8_t depth2 = lower; depth2 <= maxPhase2; ++depth2) {
    if (searchPhase2(edge, esym, corn, csym, mid, depth2,
                     phase1Length, 10)) {
      return true;
    }
    if (depth2 == 255) break;
  }
  return false;
}

bool Min2PhaseSolver::searchPhase2(uint16_t edge, uint8_t esym,
                                   uint16_t corn, uint8_t csym,
                                   uint8_t mid, uint8_t remaining,
                                   uint8_t depth, uint8_t lastMove) {
  if (edge == 0 && corn == 0 && mid == 0) {
    solutionLength_ = depth;
    return true;
  }
  if (remaining == 0) return false;

  const uint16_t moveMask = phase2MoveMask_[lastMove];
  for (uint8_t move = 0; move < kNMoves2; ++move) {
    if ((moveMask & (1u << move)) != 0) continue;

    const uint8_t midx = tables_->mPermMove[mid * kNMoves2 + move];

    uint16_t cornPacked = tables_->cPermMove[
        corn * kNMoves2 + tables_->symMoveUD[csym * kNMoves + move]];
    const uint8_t csymx = tables_->symMult[(cornPacked & 0xfu) * 16 + csym];
    const uint16_t cornx = cornPacked >> 4;

    if (pruning(tables_->mcPermPrun,
                cornx * kNMPerm + tables_->mPermConj[midx * 16 + csymx]) >=
        remaining) {
      continue;
    }

    uint16_t edgePacked = tables_->ePermMove[
        edge * kNMoves2 + tables_->symMoveUD[esym * kNMoves + move]];
    const uint8_t esymx = tables_->symMult[(edgePacked & 0xfu) * 16 + esym];
    const uint16_t edgex = edgePacked >> 4;

    const uint8_t comb = tables_->perm2CombP[cornx] & 0xffu;
    const uint8_t relation = tables_->symMultInv[esymx * 16 + csymx];
    if (pruning(tables_->ePermCCombPPrun,
                edgex * kNComb +
                    tables_->cCombPConj[comb * 16 + relation]) >= remaining) {
      continue;
    }

    const uint16_t edgeInv = getPermSymInv(edgex, esymx, false);
    const uint16_t cornInv = getPermSymInv(cornx, csymx, true);
    const uint8_t inverseComb = tables_->perm2CombP[cornInv >> 4] & 0xffu;
    const uint8_t inverseRelation = tables_->symMultInv[
        (edgeInv & 0xfu) * 16 + (cornInv & 0xfu)];
    if (pruning(tables_->ePermCCombPPrun,
                (edgeInv >> 4) * kNComb +
                    tables_->cCombPConj[inverseComb * 16 + inverseRelation]) >=
        remaining) {
      continue;
    }

    moves_[depth] = kUd2Std[move];
    if (searchPhase2(edgex, esymx, cornx, csymx, midx,
                     remaining - 1, depth + 1, move)) {
      return true;
    }
  }
  return false;
}

void Min2PhaseSolver::formatSolution(uint8_t length,
                                     Min2PhaseSolveResult& result) const {
  result.moves[0] = '\0';
  size_t used = 0;
  for (uint8_t i = 0; i < length; ++i) {
    const char* token = kMoveNames[moves_[i]];
    const size_t tokenLength = std::strlen(token);
    const size_t extra = tokenLength + (i == 0 ? 0 : 1);
    if (used + extra + 1 > sizeof(result.moves)) break;
    if (i != 0) result.moves[used++] = ' ';
    std::memcpy(result.moves + used, token, tokenLength);
    used += tokenLength;
    result.moves[used] = '\0';
  }
}

Min2PhaseSolveResult Min2PhaseSolver::solve(const char* facelets,
                                             uint8_t maxDepth) {
  Min2PhaseSolveResult result;
  const uint32_t started = millis();
  if (!ready_) {
    result.error = -9;
    return result;
  }

  CubieCube input;
  const int validation = verifyCube(input, facelets);
  if (validation != 0) {
    result.error = validation;
    result.elapsedMs = millis() - started;
    return result;
  }

  maxDepth_ = maxDepth;
  solutionLength_ = 0;
  cubePath_[0] = input;
  coordPath_[0] = makeCoord(input);

  const uint8_t firstDepth = coordPath_[0].prun;
  const uint8_t maxPhase1 = std::min<uint8_t>(kMaxPhase1Depth, maxDepth_);
  for (uint8_t depth = firstDepth; depth <= maxPhase1; ++depth) {
    phase1Length_ = depth;
    if (searchPhase1(depth, -30)) {
      result.success = true;
      result.length = solutionLength_;
      formatSolution(solutionLength_, result);
      result.elapsedMs = millis() - started;
      return result;
    }
    if (depth == 255) break;
  }

  result.error = -7;
  result.elapsedMs = millis() - started;
  return result;
}

bool Min2PhaseSolver::parseMoves(const char* sequence, CubieCube& cube) const {
  if (!sequence) return false;
  const char* cursor = sequence;
  CubieCube temp;

  while (*cursor != '\0') {
    while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n') {
      ++cursor;
    }
    if (*cursor == '\0') break;

    const int face = faceIndex(*cursor++);
    if (face < 0) return false;
    uint8_t power = 0;
    if (*cursor == '2') {
      power = 1;
      ++cursor;
    } else if (*cursor == '\'') {
      power = 2;
      ++cursor;
    }

    const uint8_t move = static_cast<uint8_t>(face * 3 + power);
    applyMove(cube, move, temp);
    cube = temp;
  }
  return true;
}

bool Min2PhaseSolver::parseScramble(const char* sequence, CubieCube& cube) const {
  cube.reset();
  return parseMoves(sequence, cube);
}

bool Min2PhaseSolver::isSolved(const CubieCube& cube) const {
  for (uint8_t i = 0; i < 8; ++i) {
    if (cube.ca[i] != i) return false;
  }
  for (uint8_t i = 0; i < 12; ++i) {
    if (cube.ea[i] != i) return false;
  }
  return true;
}

void Min2PhaseSolver::toFacelets(const CubieCube& cube, char out[55]) const {
  constexpr char faces[] = "URFDLB";
  for (uint8_t i = 0; i < 54; ++i) out[i] = faces[i / 9];

  for (uint8_t c = 0; c < 8; ++c) {
    const uint8_t cubie = cube.ca[c] & 0xfu;
    const uint8_t ori = cube.ca[c] >> 4;
    for (uint8_t n = 0; n < 3; ++n) {
      out[kCornerFacelet[c][(n + ori) % 3]] =
          faces[kCornerFacelet[cubie][n] / 9];
    }
  }
  for (uint8_t e = 0; e < 12; ++e) {
    const uint8_t cubie = cube.ea[e] & 0xfu;
    const uint8_t ori = cube.ea[e] >> 4;
    for (uint8_t n = 0; n < 2; ++n) {
      out[kEdgeFacelet[e][(n + ori) % 2]] =
          faces[kEdgeFacelet[cubie][n] / 9];
    }
  }
  out[54] = '\0';
}

bool Min2PhaseSolver::selfTest(Stream& out) {
  if (!ready_) {
    out.println("Min2Phase self-test: SKIPPED (not ready)");
    return false;
  }

  constexpr char kSolved[] =
      "UUUUUUUUURRRRRRRRRFFFFFFFFFDDDDDDDDDLLLLLLLLLBBBBBBBBB";

  out.println();
  out.println("Phase 2 - Min2Phase solver core self-test");

  const Min2PhaseSolveResult solved = solve(kSolved);
  const bool solvedPass = solved.success && solved.length == 0;
  out.printf("Solved state: %s (%u ms, length=%u)\n",
             solvedPass ? "PASS" : "FAIL",
             static_cast<unsigned>(solved.elapsedMs),
             static_cast<unsigned>(solved.length));
  if (!solvedPass) {
    out.printf("Solved error: %d\n", solved.error);
    return false;
  }

  constexpr char kScramble[] = "R U R' U'";
  CubieCube scrambled;
  if (!parseScramble(kScramble, scrambled)) {
    out.println("Known scramble setup: FAIL");
    return false;
  }

  char facelets[55];
  toFacelets(scrambled, facelets);
  const Min2PhaseSolveResult result = solve(facelets);

  out.printf("Known scramble: %s\n", kScramble);
  out.printf("Known facelets: %s\n", facelets);
  out.printf("Solve result: %s\n", result.success ? result.moves : "FAILED");
  out.printf("Solve time: %u ms\n", static_cast<unsigned>(result.elapsedMs));
  out.printf("Solution length: %u\n", static_cast<unsigned>(result.length));
  if (!result.success) {
    out.printf("Solve error: %d\n", result.error);
    return false;
  }

  CubieCube verified = scrambled;
  const bool parsed = parseMoves(result.moves, verified);
  const bool pass = parsed && isSolved(verified);
  out.printf("Solution verification: %s\n", pass ? "PASS" : "FAIL");
  return pass;
}

}  // namespace solver
