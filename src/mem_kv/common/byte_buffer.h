// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

#include "mem_kv/common/slice.h"

namespace jkv {

class ByteBuffer {
 public:
  ByteBuffer();
  ~ByteBuffer() = default;

  void Put(const uint8_t* data, uint32_t len);
  void ReadBytes(uint32_t bytes);

  bool Readable() const { return write_offset_ > read_offset_; }
  uint32_t ReadableBytes() const;
  uint32_t Capacity() const { return static_cast<uint32_t>(buf_.capacity()); }
  const uint8_t* Reader() const { return buf_.data() + read_offset_; }

  Slice GetSlice() const {
    return Slice((const char*)Reader(), ReadableBytes());
  }

  void Reset();

 private:
  void MayShrinkToFit();

  uint32_t read_offset_;
  uint32_t write_offset_;
  std::vector<uint8_t> buf_;
};

}  // namespace jkv
