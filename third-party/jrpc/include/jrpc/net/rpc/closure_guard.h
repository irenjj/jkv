// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <google/protobuf/service.h>

#include <utility>

namespace jrpc {

// RAII: Call Run() of the closure on destruction.
class ClosureGuard {
 public:
  ClosureGuard() : done_(nullptr) {}

  explicit ClosureGuard(google::protobuf::Closure* done) : done_(done) {}

  ~ClosureGuard() {
    if (done_) {
      done_->Run();
    }
  }

  void reset(google::protobuf::Closure* done) {
    if (done_) {
      done_->Run();
    }
    done_ = done;
  }

  google::protobuf::Closure* release() {
    google::protobuf::Closure* prev_done = done_;
    done_ = nullptr;
    return prev_done;
  }

  bool empty() const { return done_ == nullptr; }

  void swap(ClosureGuard& other) { std::swap(done_, other.done_); }

 private:
  google::protobuf::Closure* done_;
};

}  // namespace jrpc
