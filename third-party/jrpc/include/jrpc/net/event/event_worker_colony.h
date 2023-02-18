// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/memory/memory_pool.h"
#include "base/noncopyable.h"
#include "net/sock_address.h"
#include "net/types.h"

namespace jrpc {

class ConnectedSocket;
class ServerSocket;
class EventLoop;

class EventWorker : public jbase::noncopyable {
 public:
  EventWorker(uint32_t i, const CreateUserHandleCb& cb,
              const WorkerOption& option);
  ~EventWorker();

  void Init();
  void InitDone();
  bool IsInit();
  void WaitForInit();

  void Running(const std::string& worker_name);
  void Stop();

  void* Malloc(size_t len);
  void Free(void* ptr);

  int Listen(const SockAddress& sa, std::shared_ptr<ServerSocket>* sock);
  int Connect(const SockAddress& addr,
              std::shared_ptr<ConnectedSocket>* socket);

  int GetMrKey(void* addr, uint32_t* lkey, uint32_t* rkey);
  int RegisterMr(void* addr, size_t size);
  int UnregisterMr(void* addr, size_t size);

  void IncRefs() { refs_.fetch_add(1); }
  void DecRefs() { refs_.fetch_sub(1); }
  int GetRefs() { return refs_.load(); }

  void AddObjPoolInfo(const std::string& obj_name, int total_cnt, int free_cnt);

  EventLoop* loop() const { return loop_; }
  std::string name() const { return name_; }
  const WorkerOption& option() const { return option_; }
  int id() const { return id_; }
  bool done() const { return done_; }

 private:
  friend class EventWorkerColony;

  void GetMemPoolStat(jbase::MemPoolStat* stats);
  void ConstructMemPoolSizeMap(jbase::MemPoolSizeMap* size_map) const;
  void ExtendMemoryPool();
  void ShrinkMemoryPool();

  void Report();
  std::string ReportMemPoolInfo();
  std::string ReportObjPoolInfo();

  uint32_t id_;
  CreateUserHandleCb create_user_handle_cb_;
  WorkerOption option_;
  std::atomic<int> refs_;
  bool init_;
  bool running_;
  bool done_;
  EventLoop* loop_;
  std::string name_;
  std::mutex init_lock_;
  std::condition_variable init_cond_;
  std::list<jbase::MemoryPool*> mem_pools_;
  // obj_name => <obj_total_cnt, obj_free_cnt>
  std::unordered_map<std::string, std::pair<int, int>> obj_pool_info_;
};

class EventWorkerColony : public jbase::noncopyable {
 public:
  EventWorkerColony(uint32_t worker_num, const WorkerOption& option,
                    const CreateUserHandleCb& cb);
  ~EventWorkerColony();

  void Start();
  void Stop();

  EventWorker* GetWorker();
  EventWorker* GetWorker(uint32_t i);
  EventWorker* GetHashWorker(uint64_t id);
  const std::vector<EventWorker*>& GetAllWorkers() const;

  uint32_t worker_num() const { return worker_num_; }
  const std::atomic<bool>& started() { return started_; }
  const WorkerOption& option() const { return option_; }

 private:
  EventWorker* GetNextWorker();
  EventWorker* GetEmptyWorker();
  EventWorker* GetLightestWorker();

  void SpawnWorker(uint32_t i, std::function<void()>&& func);
  void JoinWorker(uint32_t i);
  std::function<void()> CreateThreadFunc(uint32_t i);

  uint32_t worker_num_;
  WorkerOption option_;
  std::atomic<bool> started_;
  std::vector<EventWorker*> workers_;
  int next_ = 0;
  std::mutex mutex_;
  std::vector<std::thread> threads_;
  std::vector<std::function<void()>> funcs_;
};

}  // namespace jrpc
