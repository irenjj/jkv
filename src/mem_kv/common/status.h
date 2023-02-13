// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <string>

#include "mem_kv/common/slice.h"

namespace jkv {

class Status {
 public:
  // Create a success status.
  Status() noexcept : state_(nullptr) {}
  ~Status() { delete[] state_; }

  Status(const Status& rhs);
  Status& operator=(const Status& rhs);
  Status(Status&& rhs) noexcept;
  Status& operator=(Status&& rhs) noexcept;

  static Status OK();
  static Status NotFound(const Slice& msg, const Slice& msg2 = Slice());
  static Status NotSupported(const Slice& msg, const Slice& msg2 = Slice());
  static Status InvalidArgument(const Slice& msg, const Slice& msg2 = Slice());
  static Status IOError(const Slice& msg, const Slice& msg2 = Slice());

  bool ok() const;
  bool IsNotFound() const;
  bool IsIOError() const;
  bool IsNotSupportedError() const;
  bool IsInvalidArgument() const;

  std::string ToString() const;

 private:
  enum Code {
    kOk = 0,
    kNotFound = 1,
    kNotSupported = 2,
    kInvalidArgument = 3,
    kIOError = 4,
  };

  Code code() const;

  Status(Code code, const Slice& msg, const Slice& msg2);
  static const char* CopyState(const char* s);

  // OK status has a null state_.  Otherwise, state_ is a new[] array
  // of the following form:
  //    state_[0..3] == length of message
  //    state_[4]    == code
  //    state_[5..]  == message
  const char* state_;
};

}  // namespace jkv
