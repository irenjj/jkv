// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <cassert>
#include <vector>

#include "base/thread/current_thread.h"
#include "net/event/timer.h"
#include "net/types.h"

namespace jrpc {

class LibeventDriver;
class EventTimerQueue;
class EventWorker;
class LoopTask;

class EventLoop {
 public:
  EventLoop(EventWorker* w, const CreateUserHandleCb& cb);
  ~EventLoop();

  int Init();

  void AssertInLoopThread() const;
  inline bool IsInLoopThread() const {
    return thread_id_ == jbase::CurrentThread::tid();
  }

  TimerId RunAt(jbase::TimeCycle time, const TimerCb& cb);
  TimerId RunAfter(double delay, const TimerCb& cb);
  TimerId RunEvery(double interval, const TimerCb& cb);
  void CancelTimer(const TimerId& timer_id);

  void CreateUserHandle();
  int DoPendingTasks();

  void RunInLoop(const LoopTaskCb& cb, bool async = true);
  void RunNextLoop(const LoopTaskCb& cb);

  void ProcessEvents(bool nowait);
  int RegisterEvent(int fd, int what, EventCbFunc cb, void* arg);
  void UnregisterEvent(int idx);

  EventWorker* worker() const { return worker_; }
  pid_t thread_id() const { return thread_id_; }
  int thread_no() const { return thread_no_; }
  UserHandle* user_handle() const { return user_handle_; }
  LibeventDriver* driver() const { return driver_; }

 private:
  void QueueInLoop(LoopTask* task);
  bool QueueInLoopInner(LoopTask* task);
  bool DoPendingTasksInner(size_t* do_cnt);

  int Wakeup() const;
  static void WakeupEventCbWrapper(int fd, short event, void* arg);
  int WakeupHandle();

  EventWorker* worker_;
  pid_t thread_id_;
  int thread_no_;
  UserHandle* user_handle_;
  CreateUserHandleCb create_user_handle_cb_;
  EventTimerQueue* timer_queue_;

  LibeventDriver* driver_;
  int wakeup_fd_;
  int wakeup_event_idx_;

  SpinLock lock_;
  std::vector<LoopTask*> pending_tasks_;
};

}  // namespace jrpc
