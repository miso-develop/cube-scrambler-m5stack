#pragma once

#include <Arduino.h>

namespace solver {

constexpr uint16_t kSolverTableFormatVersion = 1;
constexpr size_t kSolverTableMaxSections = 32;
constexpr char kSolverTableMagic[8] = {'C', 'S', 'N', '2', 'T', 'A', 'B', '\0'};

#pragma pack(push, 1)
struct SolverTableImageHeader {
  char magic[8];
  uint16_t formatVersion;
  uint16_t headerSize;
  uint32_t imageSize;
  uint32_t payloadOffset;
  uint32_t payloadSize;
  uint32_t payloadCrc32;
  uint32_t sectionCount;
  char sourceRevision[40];
  uint32_t reserved;
};

struct SolverTableSectionEntry {
  char name[16];
  uint32_t offset;
  uint32_t size;
  uint32_t elementSize;
  uint32_t flags;
};
#pragma pack(pop)

static_assert(sizeof(SolverTableImageHeader) == 76,
              "Unexpected SolverTableImageHeader layout");
static_assert(sizeof(SolverTableSectionEntry) == 32,
              "Unexpected SolverTableSectionEntry layout");

struct SolverTableSectionView {
  const uint8_t* data = nullptr;
  size_t size = 0;
  uint32_t elementSize = 0;
  uint32_t flags = 0;
};

uint32_t solverTableCrc32(const uint8_t* data, size_t size);

}  // namespace solver
