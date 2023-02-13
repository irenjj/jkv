// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

#include "mem_kv/wal/wal_option.h"

namespace jkv {

class WalFile {
 public:
  WalFile(const char* path, uint64_t seq);
  ~WalFile();

  void Append(WalType type, const uint8_t* data, size_t len);
  void Truncate(size_t offset);

  void Flush();
  void Read(std::vector<char>* out);

  size_t file_size() const { return file_size_; }

 private:
  std::vector<uint8_t> data_buffer_;
  uint64_t seq_;
  size_t file_size_;
  FILE* fp_;
};

using WalFilePtr = std::shared_ptr<WalFile>;

}  // namespace jkv
