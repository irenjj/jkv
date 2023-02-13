// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <memory>
#include <vector>

namespace jbase {

class MemorySlab {
 public:
  static std::unique_ptr<MemorySlab> Make(void *addr, size_t chunk_size,
                                          size_t cnt);

  MemorySlab(void *addr, size_t chunk_size, size_t cnt);
  ~MemorySlab();

  MemorySlab(const MemorySlab &) = delete;
  MemorySlab &operator=(const MemorySlab &) = delete;

  void *Malloc();
  void Free(void *p);

  size_t left_count() const { return free_list_size_; }
  size_t chunk_size() const { return chunk_size_; }
  void *ptr() const { return ptr_; }
  size_t count() const { return count_; }

 private:
  bool FreeCheckAndSet(void *p);
  bool MallocCheckAndSet(void *p);
  int64_t GetIndex(void *p) const;
  bool Build();

  void *ptr_;
  size_t chunk_size_;
  size_t count_;
  void **free_list_ = nullptr;
  size_t free_list_size_ = 0;
  std::vector<bool> bitmap_;

  uint64_t *account_malloc_cnt_ = nullptr;
  uint64_t *account_free_cnt_ = nullptr;
};

}  // namespace jbase
