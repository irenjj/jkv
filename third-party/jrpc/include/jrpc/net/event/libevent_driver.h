// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <event2/event.h>

#include <deque>
#include <vector>

#include "net/types.h"

namespace jrpc {

class LibeventDriver {
 public:
  struct EventInfo {
    struct event* event;
    bool is_enable;

    EventInfo() : event(nullptr), is_enable(false) {}
    ~EventInfo() { event_free(event); }
  };

  static const int kEventNum = 10000;

  LibeventDriver();
  ~LibeventDriver();

  int Init();
  void Stop();
  int CreateEvent(int fd, int what, EventCbFunc cb, void* arg);
  int FreeEvent(int idx);
  int AddEvent(int idx);
  int DelEvent(int idx);
  void EventWait(bool nowait);
  bool EventIsEnable(int idx);

 private:
  struct event_base* base_ = nullptr;
  std::vector<EventInfo*> events_;
  std::deque<int> free_idx_;
};

}  // namespace jrpc
