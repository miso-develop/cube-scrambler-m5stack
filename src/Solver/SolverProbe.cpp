#include "SolverProbe.h"

#include "esp_heap_caps.h"

namespace solver {
namespace {

struct TableFootprint {
  const char* name;
  size_t bytes;
};

constexpr size_t kNMoves = 18;
constexpr size_t kNMoves2 = 10;
constexpr size_t kSym = 16;
constexpr size_t kSymClasses = 8;
constexpr size_t kNSlice = 495;
constexpr size_t kNTwist = 2187;
constexpr size_t kNFlip = 2048;
constexpr size_t kNPerm = 40320;
constexpr size_t kNMPerm = 24;
constexpr size_t kNComb = 140;
constexpr size_t kNTwistSym = 324;
constexpr size_t kNFlipSym = 336;
constexpr size_t kNPermSym = 2768;

constexpr TableFootprint kMajorTables[] = {
    {"FlipS2RF", kNFlipSym * kSymClasses * sizeof(uint16_t)},
    {"EPermR2S", kNPerm * sizeof(uint16_t)},
    {"FlipR2S", kNFlip * sizeof(uint16_t)},
    {"TwistR2S", kNTwist * sizeof(uint16_t)},
    {"FlipMove", kNFlipSym * kNMoves * sizeof(uint16_t)},
    {"TwistMove", kNTwistSym * kNMoves * sizeof(uint16_t)},
    {"UDSliceMove", kNSlice * kNMoves * sizeof(uint16_t)},
    {"UDSliceConj", kNSlice * kSymClasses * sizeof(uint16_t)},
    {"UDSliceTwistPrun",
     (kNSlice * kNTwistSym / kSymClasses + 1) * sizeof(int32_t)},
    {"UDSliceFlipPrun",
     (kNSlice * kNFlipSym / kSymClasses + 1) * sizeof(int32_t)},
    {"TwistFlipPrun",
     (kNFlip * kNTwistSym / kSymClasses + 1) * sizeof(int32_t)},
    {"CPermMove", kNPermSym * kNMoves2 * sizeof(uint16_t)},
    {"EPermMove", kNPermSym * kNMoves2 * sizeof(uint16_t)},
    {"MPermMove", kNMPerm * kNMoves2 * sizeof(uint8_t)},
    {"MPermConj", kNMPerm * kSym * sizeof(uint8_t)},
    {"CCombPConj", kNComb * kSym * sizeof(uint8_t)},
    {"MCPermPrun",
     (kNMPerm * kNPermSym / kSymClasses + 1) * sizeof(int32_t)},
    {"EPermCCombPPrun",
     (kNComb * kNPermSym / kSymClasses + 1) * sizeof(int32_t)},
    {"CCombPMove", kNComb * kNMoves2 * sizeof(uint8_t)},
};

constexpr size_t calculateMajorTableBytes() {
  size_t total = 0;
  for (const auto& table : kMajorTables) {
    total += table.bytes;
  }
  return total;
}

constexpr size_t kMajorTableBytes = calculateMajorTableBytes();

}  // namespace

size_t majorTableFootprintBytes() {
  return kMajorTableBytes;
}

void printFeasibilityBaseline(Stream& out) {
  const size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  const size_t minFreeHeap = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
  const size_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

  out.println();
  out.println("Phase 2 - Solver feasibility baseline");
  out.printf("Free 8-bit heap: %u bytes\n", static_cast<unsigned>(freeHeap));
  out.printf("Minimum free 8-bit heap: %u bytes\n",
             static_cast<unsigned>(minFreeHeap));
  out.printf("Largest free 8-bit block: %u bytes\n",
             static_cast<unsigned>(largestBlock));
  out.printf("Major Min2Phase table footprint: %u bytes (~%u KiB)\n",
             static_cast<unsigned>(kMajorTableBytes),
             static_cast<unsigned>((kMajorTableBytes + 1023) / 1024));
  out.println("Table allocation policy: FLASH_REQUIRED");
  out.println("Solver backend: INTEGRATION_POC");
}

}  // namespace solver
