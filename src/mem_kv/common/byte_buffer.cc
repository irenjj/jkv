// Copyright (c) renjj - All Rights Reserved
#include "mem_kv/common/byte_buffer.h"

namespace jkv {

constexpr uint32_t kMinBufferRing = 4096;  // 4 kb

ByteBuffer::ByteBuffer()
    : read_offset_(0), write_offset_(0), buf_(kMinBufferRing) {}

void ByteBuffer::Put(const uint8_t* data, uint32_t len) {
  uint32_t left = static_cast<uint32_t>(buf_.size()) - write_offset_;
  if (left < len) {
    buf_.resize(buf_.size() * 2 + len, 0);
  }
  memcpy(buf_.data() + write_offset_, data, len);
  write_offset_ += len;
}

void ByteBuffer::ReadBytes(uint32_t bytes) {
  assert(ReadableBytes() >= bytes);
  read_offset_ += bytes;
  MayShrinkToFit();
}

uint32_t ByteBuffer::ReadableBytes() const {
  assert(write_offset_ >= read_offset_);
  return write_offset_ - read_offset_;
}

void ByteBuffer::Reset() {
  read_offset_ = 0;
  write_offset_ = 0;
  buf_.resize(kMinBufferRing);
  buf_.shrink_to_fit();
}

void ByteBuffer::MayShrinkToFit() {
  if (read_offset_ == write_offset_) {
    read_offset_ = 0;
    write_offset_ = 0;
  }
}

}  // namespace jkv
