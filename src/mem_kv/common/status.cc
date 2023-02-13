// Copyright (c) renjj - All Rights Reserved
#include "mem_kv/common/status.h"

namespace jkv {

Status::Status(const Status& rhs) {
  state_ = (rhs.state_ == nullptr) ? nullptr : CopyState(rhs.state_);
}

Status& Status::operator=(const Status& rhs) {
  // The following condition catches both aliasing (when this == &rhs),
  // and the common case where both rhs and *this are ok.
  if (state_ != rhs.state_) {
    delete[] state_;
    state_ = (rhs.state_ == nullptr) ? nullptr : CopyState(rhs.state_);
  }
  return *this;
}

Status::Status(Status&& rhs) noexcept : state_(rhs.state_) {
  rhs.state_ = nullptr;
}

Status& Status::operator=(Status&& rhs) noexcept {
  std::swap(state_, rhs.state_);
  return *this;
}

// Return a success status.
Status Status::OK() { return Status(); }

// Return error status of an appropriate type.
Status Status::NotFound(const Slice& msg, const Slice& msg2) {
  return Status(kNotFound, msg, msg2);
}

Status Status::NotSupported(const Slice& msg, const Slice& msg2) {
  return Status(kNotSupported, msg, msg2);
}

Status Status::InvalidArgument(const Slice& msg, const Slice& msg2) {
  return Status(kInvalidArgument, msg, msg2);
}
Status Status::IOError(const Slice& msg, const Slice& msg2) {
  return Status(kIOError, msg, msg2);
}

// Returns true iff the status indicates success.
bool Status::ok() const { return (state_ == nullptr); }

// Returns true iff the status indicates a NotFound error.
bool Status::IsNotFound() const { return code() == kNotFound; }

// Returns true iff the status indicates an IOError.
bool Status::IsIOError() const { return code() == kIOError; }

// Returns true iff the status indicates a NotSupportedError.
bool Status::IsNotSupportedError() const { return code() == kNotSupported; }

// Returns true iff the status indicates an InvalidArgument.
bool Status::IsInvalidArgument() const { return code() == kInvalidArgument; }

// Return a string representation of this status suitable for printing.
// Returns the string "OK" for success.
std::string Status::ToString() const {
  if (state_ == nullptr) {
    return "OK";
  } else {
    char tmp[30];
    const char* type;
    switch (code()) {
      case kOk:
        type = "OK";
        break;
      case kNotFound:
        type = "NotFound: ";
        break;
      case kNotSupported:
        type = "Not implemented: ";
        break;
      case kInvalidArgument:
        type = "Invalid argument: ";
        break;
      case kIOError:
        type = "IO error: ";
        break;
      default:
        std::snprintf(tmp, sizeof(tmp),
                      "Unknown code(%d): ", static_cast<int>(code()));
        type = tmp;
        break;
    }
    std::string result(type);
    uint32_t length;
    std::memcpy(&length, state_, sizeof(length));
    result.append(state_ + 5, length);
    return result;
  }
}

Status::Status(Code code, const Slice& msg, const Slice& msg2) {
  assert(code != kOk);
  const auto len1 = static_cast<uint32_t>(msg.size());
  const auto len2 = static_cast<uint32_t>(msg2.size());
  const uint32_t size = len1 + (len2 ? (2 + len2) : 0);
  char* result = new char[size + 5];
  std::memcpy(result, &size, sizeof(size));
  result[4] = static_cast<char>(code);
  std::memcpy(result + 5, msg.data(), len1);
  if (len2) {
    result[5 + len1] = ':';
    result[6 + len1] = ' ';
    std::memcpy(result + 7 + len1, msg2.data(), len2);
  }
  state_ = result;
}

Status::Code Status::code() const {
  return (state_ == nullptr) ? kOk : static_cast<Code>(state_[4]);
}

const char* Status::CopyState(const char* state) {
  uint32_t size;
  std::memcpy(&size, state, sizeof(size));
  char* result = new char[size + 5];
  std::memcpy(result, state, size + 5);
  return result;
}

}  // namespace jkv
