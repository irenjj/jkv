// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <jraft/memory_storage.h>
#include <jraft/rawnode.h>

#include <boost/asio.hpp>

#include "eraft.pb.h"
#include "mem_kv/common/config.h"

namespace jkv {

class PeerStore;
using PeerStorePtr = std::shared_ptr<PeerStore>;
class Transport;
using TransportPtr = std::shared_ptr<Transport>;
class Snapper;
class Wal;
using WalPtr = std::shared_ptr<Wal>;
using EntryPtr = std::shared_ptr<jraft::Entry>;
class Status;
using MessagePtr = std::shared_ptr<jraft::Message>;
using SnapshotPtr = std::shared_ptr<jraft::Snapshot>;

class Peer {
 public:
  Peer(uint64_t id, uint64_t port, const std::string& cluster);
  ~Peer();

  void Start();
  void Stop();

  void Propose(const std::string& data, const StatusCallback& cb);
  void Process(MessagePtr msg, const StatusCallback& cb);

  void IsIdRemoved(uint64_t id, const std::function<void(bool)>& cb);
  void ReportUnreachable(uint64_t id);
  void ReportSnapshot(uint64_t id, SnapshotState state);

  bool PublishEntries(const std::vector<EntryPtr>& ents);
  void EntriesToApply(const std::vector<EntryPtr>& total_ents,
                      std::vector<EntryPtr>* ents_to_apply);

  void MaybeTriggerSnapshot();

  uint64_t id() const { return id_; }

 private:
  void ReplayWal();
  void OpenWal(const jraft::Snapshot& snap);

  void StartTimer();
  void PullReadyEvents();
  Status SaveSnap(const jraft::Snapshot& snap);
  void PublishSnap(const jraft::Snapshot& snap);

  void Schedule();

  uint64_t id_;
  uint16_t port_;
  pthread_t tid_;
  uint64_t last_index_;
  std::unique_ptr<jraft::RawNode> raw_node_;
  std::vector<std::string> peers_;
  uint64_t snap_index_;
  uint64_t applied_index_;
  jraft::ConfState conf_state_;

  boost::asio::io_service io_service_;
  boost::asio::deadline_timer timer_;
  TransportPtr transport_;

  jraft::MemoryStoragePtr storage_;
  PeerStorePtr peer_store_;

  std::string snap_dir_;
  std::string snap_data_;
  uint64_t snap_count_;
  std::unique_ptr<Snapper> snapper_;

  std::string wal_dir_;
  WalPtr wal_;
};

using PeerPtr = std::shared_ptr<Peer>;

}  // namespace jkv
