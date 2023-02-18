// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <pthread.h>

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>

#include "base/fixed_size_function.h"

#define kProtocolName "JRPC"

#define PALIGN_DOWN(x, align) (x & ~(align - 1))
#define PALIGN_UP(x, align) ((x + (align - 1)) & ~(align - 1))

#define JRPC__CACHELINE_SIZE 64

#ifdef __GNUC__
#define JRPC_CACHELINE_ALIGNMENT __attribute__((aligned(JRPC_CACHELINE_SIZE)))
#endif /* __GNUC__ */

#ifndef JRPC_CACHELINE_ALIGNMENT
#define JRPC_CACHELINE_ALIGNMENT /*JRPC_CACHELINE_ALIGNMENT*/
#endif

namespace jrpc {

class TcpConnection;
class EventLoop;
class EventWorker;
class UserHandle;

class SpinLock {
 public:
  SpinLock() { pthread_spin_init(&l_, PTHREAD_PROCESS_PRIVATE); }
  ~SpinLock() { pthread_spin_destroy(&l_); }
  void lock() { pthread_spin_lock(&l_); }
  void unlock() { pthread_spin_unlock(&l_); }

 private:
  pthread_spinlock_t l_;
};

enum Transport { kTcp = 0 };

struct AppOption {
  struct LogOption {
    std::string path;
    std::string level = "info";
    size_t roll_size_mb = 50;
  } log_option;
  int numa_id = -1;
};

struct WorkerOption {
  enum WorkerSelectStrategy {
    kRoundRobin,
    kEmptyOne,
    kLightestOne,
  };
  WorkerSelectStrategy worker_strategy = kRoundRobin;
  std::string worker_name_prefix = "worker-";
  bool enable_mempool = false;
  uint32_t obj_pool_size = 16384;
};

typedef std::shared_ptr<TcpConnection> ConnectionPtr;

typedef std::function<void()> TimerCb;
typedef std::function<void()> LoopTaskCb;
typedef std::function<void()> DestroyCb;

typedef std::function<void(const ConnectionPtr&)> ConnectionSuccessCb;
typedef std::function<void(const ConnectionPtr&)> ConnectionClosedCb;
typedef std::function<void(const ConnectionPtr&)> MessageReadCb;
typedef std::function<void(const ConnectionPtr&)> MessageWriteCb;

typedef void (*EventCbFunc)(int fd, short event, void* arg);
typedef std::function<void(int)> DiskIOCb;
typedef fixed_size_function<void(int)> SendCompleteCb;

typedef std::function<void(EventLoop*)> ThreadInitCb;
typedef std::function<UserHandle*(EventLoop*)> CreateUserHandleCb;

extern ConnectionSuccessCb DefaultConnectionSuccessCb;
extern ConnectionClosedCb DefaultConnectionClosedCb;
extern MessageReadCb DefaultMessageReadCb;
extern MessageWriteCb DefaultMessageWriteCb;

void DefaultConnectionSuccessCbImpl(const ConnectionPtr& conn);
void DefaultConnectionClosedCbImpl(const ConnectionPtr& conn);
void DefaultMessageReadCbImpl(const ConnectionPtr& conn);
void DefaultMessageWriteCbImpl(const ConnectionPtr& conn);

void AppInit(const AppOption& option);
void AppRunUntilAskedToQuit(const std::string& name);

constexpr uint32_t kObjPoolSize = 16384;
constexpr uint32_t kMaxPollCnt = 24;
constexpr uint32_t kMaxThreadNum = 64;

extern int g_numa_id;
extern EventWorker* g_app_worker;
extern thread_local EventWorker* tl_worker;

}  // namespace jrpc
