#include "RandomCubeGenerator.h"

#include <cstring>
#include <esp_system.h>

namespace sequence {
namespace {

// Coordinate generation follows:
// cs0x7f/min2phase.js@0ba83a6177d816f72af1a45c9015349da597456a
// randomCube(), selected under the upstream MIT option documented in
// docs/THIRD_PARTY.md.
constexpr uint32_t kFactorial[13] = {
    1u, 1u, 2u, 6u, 24u, 120u, 720u,
    5040u, 40320u, 362880u, 3628800u, 39916800u, 479001600u};

constexpr uint8_t kCornerFacelet[8][3] = {
    {8, 9, 20}, {6, 18, 38}, {0, 36, 47}, {2, 45, 11},
    {29, 26, 15}, {27, 44, 24}, {33, 53, 42}, {35, 17, 51}};

constexpr uint8_t kEdgeFacelet[12][2] = {
    {5, 10}, {7, 19}, {3, 37}, {1, 46}, {32, 16}, {28, 25},
    {30, 43}, {34, 52}, {23, 12}, {21, 41}, {50, 39}, {48, 14}};

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

uint8_t getNParity(uint32_t idx, uint8_t n) {
  uint8_t parity = 0;
  for (int i = n - 2; i >= 0; --i) {
    parity ^= static_cast<uint8_t>(idx % (n - i));
    idx /= (n - i);
  }
  return parity & 1u;
}

void setTwist(uint8_t ca[8], uint16_t idx) {
  int twist = 15;
  for (int i = 6; i >= 0; --i, idx /= 3) {
    const uint8_t ori = static_cast<uint8_t>(idx % 3);
    ca[i] = static_cast<uint8_t>((ca[i] & 0xfu) | (ori << 4));
    twist -= ori;
  }
  ca[7] = static_cast<uint8_t>((ca[7] & 0xfu) | ((twist % 3) << 4));
}

void setFlip(uint8_t ea[12], uint16_t idx) {
  uint8_t parity = 0;
  for (int i = 10; i >= 0; --i, idx >>= 1) {
    ea[i] = static_cast<uint8_t>((ea[i] & 0xfu) | ((idx & 1u) << 4));
    parity ^= ea[i];
  }
  ea[11] = static_cast<uint8_t>((ea[11] & 0xfu) | (parity & 0x10u));
}

void toFacelets(const uint8_t ca[8], const uint8_t ea[12], char out[55]) {
  constexpr char faces[] = "URFDLB";
  for (uint8_t i = 0; i < 54; ++i) out[i] = faces[i / 9];

  for (uint8_t c = 0; c < 8; ++c) {
    const uint8_t cubie = ca[c] & 0xfu;
    const uint8_t ori = ca[c] >> 4;
    for (uint8_t n = 0; n < 3; ++n) {
      out[kCornerFacelet[c][(n + ori) % 3]] =
          faces[kCornerFacelet[cubie][n] / 9];
    }
  }
  for (uint8_t e = 0; e < 12; ++e) {
    const uint8_t cubie = ea[e] & 0xfu;
    const uint8_t ori = ea[e] >> 4;
    for (uint8_t n = 0; n < 2; ++n) {
      out[kEdgeFacelet[e][(n + ori) % 2]] =
          faces[kEdgeFacelet[cubie][n] / 9];
    }
  }
  out[54] = '\0';
}

}  // namespace

uint32_t RandomCubeGenerator::randomBelow(uint32_t upperExclusive) {
  if (upperExclusive <= 1u) return 0u;

  // Rejection sampling avoids modulo bias from esp_random().
  constexpr uint64_t kRange = (uint64_t{1} << 32);
  const uint64_t limit = kRange - (kRange % upperExclusive);
  uint32_t value = 0;
  do {
    value = esp_random();
  } while (static_cast<uint64_t>(value) >= limit);
  return value % upperExclusive;
}

bool RandomCubeGenerator::generate(char out[55]) {
  if (!out) return false;

  uint32_t edgePerm = 0;
  uint32_t cornerPerm = 0;
  do {
    edgePerm = randomBelow(kFactorial[12]);
    cornerPerm = randomBelow(kFactorial[8]);
  } while (getNParity(cornerPerm, 8) != getNParity(edgePerm, 12));

  const uint16_t flip = static_cast<uint16_t>(randomBelow(2048));
  const uint16_t twist = static_cast<uint16_t>(randomBelow(2187));

  uint8_t ca[8]{};
  uint8_t ea[12]{};
  for (uint8_t i = 0; i < 8; ++i) ca[i] = i;
  for (uint8_t i = 0; i < 12; ++i) ea[i] = i;

  setNPerm(ca, cornerPerm, 8);
  setTwist(ca, twist);
  setNPermFull(ea, edgePerm, 12);
  setFlip(ea, flip);
  toFacelets(ca, ea, out);
  return true;
}

}  // namespace sequence
