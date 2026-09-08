#include "SolverTableStorage.h"

#include <algorithm>
#include <cstring>

#include "esp_err.h"

namespace solver {
namespace {
constexpr char kSolverPartitionLabel[] = "solver";
}

SolverTableStorage::~SolverTableStorage() { end(); }

bool SolverTableStorage::begin(Stream& out) {
  end();

  partition_ = esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA,
      static_cast<esp_partition_subtype_t>(0x40),
      kSolverPartitionLabel);

  if (!partition_) {
    out.println("Solver Flash partition: NOT_FOUND");
    return false;
  }

  const void* mapped = nullptr;
  const esp_err_t err = esp_partition_mmap(
      partition_, 0, partition_->size, ESP_PARTITION_MMAP_DATA,
      &mapped, &mmapHandle_);

  if (err != ESP_OK) {
    out.printf("Solver Flash mmap: FAILED (%s)\n", esp_err_to_name(err));
    partition_ = nullptr;
    mmapHandle_ = 0;
    return false;
  }

  mapped_ = static_cast<const uint8_t*>(mapped);
  out.printf("Solver Flash partition: %u bytes\n",
             static_cast<unsigned>(partition_->size));
  out.println("Solver Flash mmap: OK");
  return true;
}

void SolverTableStorage::end() {
  if (mmapHandle_ != 0) {
    esp_partition_munmap(mmapHandle_);
  }
  partition_ = nullptr;
  mapped_ = nullptr;
  mmapHandle_ = 0;
  validImage_ = false;
}

const SolverTableImageHeader* SolverTableStorage::header() const {
  return reinterpret_cast<const SolverTableImageHeader*>(mapped_);
}

const SolverTableSectionEntry* SolverTableStorage::sectionDirectory() const {
  return reinterpret_cast<const SolverTableSectionEntry*>(
      mapped_ + sizeof(SolverTableImageHeader));
}

bool SolverTableStorage::validateImage(Stream& out) {
  validImage_ = false;
  if (!mapped_ || !partition_ ||
      partition_->size < sizeof(SolverTableImageHeader)) {
    out.println("Solver table image: INVALID (not mapped / too small)");
    return false;
  }

  const auto* hdr = header();
  if (std::memcmp(hdr->magic, kSolverTableMagic, sizeof(kSolverTableMagic)) != 0) {
    out.println("Solver table image: INVALID (magic)");
    return false;
  }
  if (hdr->formatVersion != kSolverTableFormatVersion) {
    out.printf("Solver table image: INVALID (version %u)\n",
               static_cast<unsigned>(hdr->formatVersion));
    return false;
  }
  if (hdr->sectionCount > kSolverTableMaxSections) {
    out.println("Solver table image: INVALID (section count)");
    return false;
  }

  const size_t minimumHeaderSize = sizeof(SolverTableImageHeader) +
      static_cast<size_t>(hdr->sectionCount) * sizeof(SolverTableSectionEntry);
  if (hdr->headerSize < minimumHeaderSize || hdr->payloadOffset < hdr->headerSize ||
      hdr->imageSize > partition_->size ||
      static_cast<uint64_t>(hdr->payloadOffset) + hdr->payloadSize > hdr->imageSize) {
    out.println("Solver table image: INVALID (bounds)");
    return false;
  }

  const auto* sections = sectionDirectory();
  for (uint32_t i = 0; i < hdr->sectionCount; ++i) {
    const uint64_t end = static_cast<uint64_t>(sections[i].offset) + sections[i].size;
    if (sections[i].offset < hdr->payloadOffset || end > hdr->imageSize ||
        sections[i].elementSize == 0) {
      out.printf("Solver table image: INVALID (section %u bounds)\n",
                 static_cast<unsigned>(i));
      return false;
    }
  }

  const uint32_t crc = solverTableCrc32(mapped_ + hdr->payloadOffset,
                                        hdr->payloadSize);
  if (crc != hdr->payloadCrc32) {
    out.printf("Solver table image: INVALID (CRC expected=0x%08X actual=0x%08X)\n",
               hdr->payloadCrc32, crc);
    return false;
  }

  validImage_ = true;
  out.println("Solver table image: VALID");
  out.printf("Format version: %u\n", static_cast<unsigned>(hdr->formatVersion));
  out.printf("Image size: %u bytes\n", static_cast<unsigned>(hdr->imageSize));
  out.printf("Payload size: %u bytes\n", static_cast<unsigned>(hdr->payloadSize));
  out.printf("Sections: %u\n", static_cast<unsigned>(hdr->sectionCount));
  out.printf("Source revision: %.40s\n", hdr->sourceRevision);
  out.printf("Payload CRC32: 0x%08X\n", hdr->payloadCrc32);
  return true;
}

bool SolverTableStorage::findSection(const char* name,
                                     SolverTableSectionView& view) const {
  view = {};
  if (!validImage_ || !name) {
    return false;
  }
  const auto* hdr = header();
  const auto* sections = sectionDirectory();
  for (uint32_t i = 0; i < hdr->sectionCount; ++i) {
    if (std::strncmp(sections[i].name, name, sizeof(sections[i].name)) == 0) {
      view.data = mapped_ + sections[i].offset;
      view.size = sections[i].size;
      view.elementSize = sections[i].elementSize;
      view.flags = sections[i].flags;
      return true;
    }
  }
  return false;
}

void SolverTableStorage::benchmark(Stream& out, size_t bytesToUse,
                                   uint32_t randomReads) const {
  if (!mapped_ || !partition_) {
    out.println("Solver Flash benchmark: SKIPPED (not mapped)");
    return;
  }

  const size_t partitionSize = static_cast<size_t>(partition_->size);
  const size_t usableBytes = std::min(bytesToUse, partitionSize);
  const size_t wordCount = usableBytes / sizeof(uint32_t);
  if (wordCount == 0) {
    out.println("Solver Flash benchmark: SKIPPED (empty range)");
    return;
  }

  const auto* words = reinterpret_cast<const uint32_t*>(mapped_);
  volatile uint32_t checksum = 0;

  const uint32_t sequentialStartUs = micros();
  for (size_t i = 0; i < wordCount; ++i) checksum ^= words[i];
  const uint32_t sequentialUs = micros() - sequentialStartUs;

  uint32_t state = 0x12345678u;
  const uint32_t randomStartUs = micros();
  for (uint32_t i = 0; i < randomReads; ++i) {
    state = state * 1664525u + 1013904223u;
    checksum ^= words[state % wordCount];
  }
  const uint32_t randomUs = micros() - randomStartUs;

  out.println();
  out.println("Phase 2 - Solver Flash mmap benchmark");
  out.printf("Mapped benchmark range: %u bytes\n", static_cast<unsigned>(usableBytes));
  out.printf("Sequential 32-bit reads: %u in %u us\n",
             static_cast<unsigned>(wordCount), static_cast<unsigned>(sequentialUs));
  out.printf("Random 32-bit reads: %u in %u us\n",
             static_cast<unsigned>(randomReads), static_cast<unsigned>(randomUs));
  if (randomReads > 0) {
    out.printf("Random average: %.3f us/read\n",
               static_cast<double>(randomUs) / randomReads);
  }
  out.printf("Benchmark checksum: 0x%08X\n", checksum);
}

}  // namespace solver
