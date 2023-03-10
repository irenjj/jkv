// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "base/memory/memory_slab.h"
#include "net/types.h"

namespace jbase {

// key: chunk_size; value: (total cnt, left cnt)
typedef std::map<size_t, std::pair<size_t, size_t>> MemPoolStat;
// key chunk_size; value: chunk cnt
typedef std::map<uint32_t, uint32_t> MemPoolSizeMap;

class MemoryPool {
 public:
  explicit MemoryPool();
  ~MemoryPool() = default;
  MemoryPool(const MemoryPool &) = delete;
  MemoryPool &operator=(const MemoryPool &) = delete;

  bool Init(const MemPoolSizeMap &size_map);
  bool AddSlab(void *addr, size_t chunk_size, int count);
  void *Malloc(size_t size);
  int Free(void *ptr);
  void Report(MemPoolStat *stats);
  bool CheckIdle();
  void Release();

  void *pool_ptr() const { return pool_ptr_; }
  size_t pool_size() const { return pool_size_; }

 private:
  void *MallocInner(size_t size);
  static void FreeInner(void *ptr);

  std::vector<uint64_t> slab_range_;
  std::vector<std::unique_ptr<MemorySlab>> slab_list_;
  std::unordered_set<uint64_t> big_chunks_;
  void *pool_ptr_ = nullptr;
  size_t pool_size_ = 0;
  size_t big_chunk_size_ = 0;
  size_t align_size_ = (4 << 10);

  uint64_t active_tick_ = 0;
};

}  // namespace jbase
