// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#include "net/types.h"

namespace jrpc {

class EventWorker;

class WorkerThread {
 public:
  WorkerThread(const std::string& name, const CreateUserHandleCb& cb,
               const WorkerOption& option = WorkerOption());
  ~WorkerThread();

  EventWorker* Start();

 private:
  void ThreadFunc();

  std::string name_;
  EventWorker* w_;
  std::thread thread_;
  std::mutex lock_;
  std::condition_variable cond_;
  CreateUserHandleCb create_user_handle_cb_;
  WorkerOption option_;
};

}  // namespace jrpc
