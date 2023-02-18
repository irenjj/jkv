// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <unordered_map>
#include <vector>

#include "base/noncopyable.h"
#include "base/time/timecycle.h"
#include "net/types.h"

namespace jrpc {

class EventLoop;
class Timer;
class TimerId;

class EventTimerQueue : public jbase::noncopyable {
 public:
  explicit EventTimerQueue(EventLoop* loop);
  ~EventTimerQueue();

  TimerId AddTimer(const TimerCb& cb, const jbase::TimeCycle& when,
                   double interval);

  void Cancel(const TimerId& timer_id);

 private:
  void CancelTimerInList();
  void DoExpiredTasks();
  void Reset();
  bool Insert(Timer* timer);
  static void TimerEventCbWrapper(int fd, short event, void* arg);
  void HandleTimerEvent();

  typedef std::multimap<jbase::TimeCycle, Timer*> TimerList;
  typedef std::unordered_map<int64_t, Timer*> ActiveTimerSet;

  EventLoop* loop_;
  // Timer list sorted by expiration
  TimerList timers_;

  ActiveTimerSet active_timers_;

  bool calling_expired_timers_;
  ActiveTimerSet canceling_timers_;

  int timerfd_;
  EventLoop* event_loop_ = nullptr;
  int timer_event_idx_;
  std::vector<Timer*> expired_timers_;
};

}  // namespace jrpc
