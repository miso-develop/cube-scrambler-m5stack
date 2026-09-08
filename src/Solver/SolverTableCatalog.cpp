#include "SolverTableCatalog.h"

namespace solver {
namespace {

struct ExpectedSection {
  const char* name;
  size_t count;
  uint32_t elementSize;
};

constexpr ExpectedSection kExpectedSections[] = {
    {"TwstMove", 324 * 18, 2},
    {"FlipMove", 336 * 18, 2},
    {"SliceMove", 495 * 18, 2},
    {"SliceConj", 495 * 8, 2},
    {"CPermMove", 2768 * 10, 2},
    {"EPermMove", 2768 * 10, 2},
    {"MPermMove", 24 * 10, 1},
    {"MPermConj", 24 * 16, 1},
    {"CCombPMove", 140 * 10, 1},
    {"CCombPConj", 140 * 16, 1},
    {"SymMult", 16 * 16, 1},
    {"SymMultInv", 16 * 16, 1},
    {"SymMove", 16 * 18, 1},
    {"SymMoveUD", 16 * 18, 1},
    {"Sym8Move", 18 * 8, 1},
    {"FlipS2R", 336, 2},
    {"FlipR2S", 2048, 2},
    {"FlipSelfSym", 336, 1},
    {"FlipS2RF", 336 * 8, 2},
    {"TwstS2R", 324, 2},
    {"TwstR2S", 2187, 2},
    {"TwstSelfSym", 324, 1},
    {"EPermS2R", 2768, 2},
    {"EPermR2S", 40320, 2},
    {"PermSelfSym", 2768, 2},
    {"Perm2CombP", 2768, 1},
    {"PermInvEdgeSym", 2768, 2},
    {"TwstFlipPrun", ((2048 * 324) >> 3) + 1, 4},
    {"SliceTwstPrun", ((495 * 324) >> 3) + 1, 4},
    {"SliceFlipPrun", ((495 * 336) >> 3) + 1, 4},
    {"MCPermPrun", ((24 * 2768) >> 3) + 1, 4},
    {"EPermCCombPPrun", ((140 * 2768) >> 3) + 1, 4},
};

static_assert(sizeof(kExpectedSections) / sizeof(kExpectedSections[0]) ==
                  kMin2PhaseTableSectionCount,
              "Min2Phase table section count mismatch");

}  // namespace

bool validateMin2PhaseTableCatalog(const SolverTableStorage& storage,
                                   Stream& out) {
  if (!storage.hasValidImage()) {
    out.println("Min2Phase table catalog: INVALID (image not valid)");
    return false;
  }

  size_t totalBytes = 0;
  for (const auto& expected : kExpectedSections) {
    SolverTableSectionView view;
    if (!storage.findSection(expected.name, view)) {
      out.printf("Min2Phase table catalog: INVALID (missing %s)\n",
                 expected.name);
      return false;
    }

    const size_t expectedBytes = expected.count * expected.elementSize;
    if (view.size != expectedBytes || view.elementSize != expected.elementSize) {
      out.printf(
          "Min2Phase table catalog: INVALID (%s size=%u/%u elem=%u/%u)\n",
          expected.name, static_cast<unsigned>(view.size),
          static_cast<unsigned>(expectedBytes),
          static_cast<unsigned>(view.elementSize),
          static_cast<unsigned>(expected.elementSize));
      return false;
    }
    totalBytes += view.size;
  }

  if (totalBytes != kMin2PhaseTableDataBytes) {
    out.printf("Min2Phase table catalog: INVALID (total=%u expected=%u)\n",
               static_cast<unsigned>(totalBytes),
               static_cast<unsigned>(kMin2PhaseTableDataBytes));
    return false;
  }

  out.println("Min2Phase table catalog: READY");
  out.printf("Min2Phase table sections: %u\n",
             static_cast<unsigned>(kMin2PhaseTableSectionCount));
  out.printf("Min2Phase table data: %u bytes\n",
             static_cast<unsigned>(totalBytes));
  return true;
}

}  // namespace solver
