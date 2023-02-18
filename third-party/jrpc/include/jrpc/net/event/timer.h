// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <atomic>
#include <utility>

#include "base/noncopyable.h"
#include "base/time/timecycle.h"
#include "net/types.h"

namespace jrpc {

// Internal class for timer id.
class Timer : public jbase::noncopyable {
 public:
  Timer(const TimerCb& cb, const jbase::TimeCycle& when, double interval)
      : cb_(std::move(cb)),
        expiration_(when),
        interval_(interval),
        repeat_(interval > 0.0),
        sequence_(created_num_++) {}

  void Run() { cb_(); }

  void Restart(jbase::TimeCycle now) {
    if (repeat_) {
      expiration_ = jbase::TimeCycle::AddTime(now, interval_);
    } else {
      expiration_ = jbase::TimeCycle::Invalid();
    }
  }

  static int64_t GetCreatedNum() { return created_num_.load(); }

  const jbase::TimeCycle& expiration() const { return expiration_; }
  bool repeat() const { return repeat_; }
  int64_t sequence() const { return sequence_; }

 private:
  TimerCb cb_;
  jbase::TimeCycle expiration_;
  double interval_;
  bool repeat_;
  int64_t sequence_;
  static std::atomic<int64_t> created_num_;
};

// An opaque identifier, for canceling Timer.
struct TimerId {
  TimerId() : timer(nullptr), sequence(0) {}
  TimerId(Timer* tm, int64_t seq) : timer(tm), sequence(seq) {}
  Timer* timer;
  int64_t sequence;
  bool operator==(const TimerId& rhs) const {
    return timer == rhs.timer && sequence == rhs.sequence;
  }
  bool operator!=(const TimerId& rhs) const { return !(*this == rhs); }
};

}  // namespace jrpc
