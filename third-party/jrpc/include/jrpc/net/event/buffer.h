// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <vector>

#include "base/logging/logging.h"

namespace jrpc {

class Buffer {
 public:
  static const size_t kInitialSize = 8192;
  static const size_t kMaxCapacity = 4 * 1024 * 1024;
  static const ssize_t kScoreIncUnit = 200;
  static const ssize_t kScoreDecUnit = 1;
  static const ssize_t kMaxScore = 2000;

  explicit Buffer(size_t initialSize = kInitialSize)
      : buffer_(initialSize),
        capacity_(initialSize),
        readerIndex_(0),
        writerIndex_(0),
        score_(0) {
    assert(ReadableBytes() == 0);
    assert(WriteableBytes() == initialSize - 1);
  }

  size_t ReadableBytes() const {
    ssize_t diff = writerIndex_ - readerIndex_;
    if (diff < 0) {
      diff += capacity_;
    }
    return diff;
  }

  size_t WriteableBytes() const { return capacity_ - 1 - ReadableBytes(); }

  ssize_t Retrieve(size_t len);
  void RetrieveAll();
  size_t Receive(void* data, size_t len);
  size_t Remove(void* data, size_t len);
  void HasWritten(size_t len);
  // 追加数据
  void Append(const char* data, size_t len);

  ssize_t ReadFd(int fd, int* saved_errno);
  ssize_t WriteFd(int fd);

 private:
  void MakeSpace(size_t len);
  void DoAppend(const char* data, size_t len);
  void TryIncScore();
  bool TryShrink();

  std::vector<char> buffer_;
  size_t capacity_;
  size_t readerIndex_;
  size_t writerIndex_;
  ssize_t score_;
};

}  // namespace jrpc
