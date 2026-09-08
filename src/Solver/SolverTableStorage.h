#pragma once

#include <Arduino.h>
#include "esp_partition.h"

#include "SolverTableFormat.h"

namespace solver {

class SolverTableStorage {
 public:
  SolverTableStorage() = default;
  ~SolverTableStorage();

  SolverTableStorage(const SolverTableStorage&) = delete;
  SolverTableStorage& operator=(const SolverTableStorage&) = delete;

  bool begin(Stream& out);
  void end();

  bool isMapped() const { return mapped_ != nullptr; }
  bool hasValidImage() const { return validImage_; }
  size_t partitionSize() const { return partition_ ? partition_->size : 0; }
  const uint8_t* data() const { return mapped_; }

  bool validateImage(Stream& out);
  bool findSection(const char* name, SolverTableSectionView& view) const;

  void benchmark(Stream& out, size_t bytesToUse,
                 uint32_t randomReads = 200000) const;

 private:
  const SolverTableImageHeader* header() const;
  const SolverTableSectionEntry* sectionDirectory() const;

  const esp_partition_t* partition_ = nullptr;
  const uint8_t* mapped_ = nullptr;
  esp_partition_mmap_handle_t mmapHandle_ = 0;
  bool validImage_ = false;
};

}  // namespace solver
