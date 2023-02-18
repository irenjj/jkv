// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <condition_variable>
#include <mutex>
#include <utility>

#include "base/time/timecycle.h"
#include "net/types.h"

namespace jrpc {

class LoopTask {
 public:
  explicit LoopTask(const LoopTaskCb& cb) : cb_(std::move(cb)) {}
  virtual ~LoopTask() {}

  virtual void DoTask() = 0;
  virtual void DestroySelf() = 0;

 protected:
  LoopTaskCb cb_;
  uint64_t begin_tick_ = 0;
  uint64_t end_tick_ = 0;
};

class LoopTaskAsync : public LoopTask {
 public:
  explicit LoopTaskAsync(const LoopTaskCb& cb) : LoopTask(cb) {}
  virtual ~LoopTaskAsync() {}

  void DoTask() override { cb_(); }
  void DestroySelf() override { delete this; }
};

class LoopTaskSync : public LoopTask {
 public:
  explicit LoopTaskSync(const LoopTaskCb& cb) : LoopTask(cb) {}
  virtual ~LoopTaskSync() {}

  void DoTask() override { cb_(); }

  void DestroySelf() override {
    std::lock_guard<std::mutex> lock(lock_);
    cond_.notify_all();
    done_ = true;
  }

  void Wait() {
    {
      std::unique_lock<std::mutex> l(lock_);
      while (!done_) {
        cond_.wait(l);
      }
    }
    delete this;
  }

 protected:
  std::mutex lock_;
  std::condition_variable cond_;
  bool done_ = false;
};

}  // namespace jrpc
