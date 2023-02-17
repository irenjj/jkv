// Copyright (c) renjj - All Rights Reserved
#include "mem_kv/peer/peer.h"

#include <jraft/common/util.h>
#include <jraft/memory_storage.h>
#include <jraft/raft.h>
#include <jraft/ready.h>
#include <jrpc/base/logging/logging.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "mem_kv/peer/peer_store.h"
#include "mem_kv/snapper/snapper.h"
#include "mem_kv/transport/transport.h"
#include "mem_kv/wal/wal.h"

namespace jkv {

Peer::Peer(uint64_t id, uint64_t port, const std::string& cluster)
    : id_(id),
      port_(port),
      tid_(0),
      last_index_(0),
      raw_node_(nullptr),
      peers_(),
      snap_index_(0),
      applied_index_(0),
      io_service_(),
      timer_(io_service_),
      transport_(),
      storage_(new jraft::MemoryStorage()),
      peer_store_(nullptr),
      snap_dir_(),
      snap_data_(),
      snap_count_(kDefaultSnapCount),
      snapper_(nullptr),
      wal_dir_(),
      wal_(nullptr) {
  boost::split(peers_, cluster, boost::is_any_of(","));
  if (peers_.empty()) {
    JLOG_FATAL << "invalid args" << cluster.c_str();
  }

  std::string work_dir = "node_" + std::to_string(id);
  snap_dir_ = work_dir + "/snap";
  wal_dir_ = work_dir + "/wal";

  if (!boost::filesystem::exists(snap_dir_)) {
    boost::filesystem::create_directories(snap_dir_);
  }

  snapper_.reset(new Snapper(snap_dir_));

  ReplayWal();

  jraft::Config c;
  c.id = id_;
  c.election_tick = 10;
  c.heartbeat_tick = 1;
  c.storage = storage_;
  c.applied = 0;
  c.max_size_per_msg = 1024 * 1024;
  c.max_committed_size_per_ready = 0;
  c.max_uncommitted_entries_size = 1 << 30;
  c.max_inflight_msgs = 256;
  c.check_quorum = true;
  c.pre_vote = true;
  c.read_only_option = jraft::kReadOnlySafe;
  c.disable_proposal_forwarding = false;

  jraft::Validate(&c);

  jraft::ConfState* storage_cs =
      storage_->mutable_snapshot()->mutable_metadata()->mutable_conf_state();
  for (int i = 0; i < peers_.size(); i++) {
    storage_cs->add_voters(i + 1);
  }

  raw_node_.reset(new jraft::RawNode(c));
}

Peer::~Peer() {
  JLOG_DEBUG << "stop peer";
  if (transport_) {
    transport_->Stop();
    transport_ = nullptr;
  }
}

void Peer::Start() {
  transport_ = std::make_shared<Transport>(this, id_);
  std::string host = peers_[id_ - 1];
  transport_->Start(host);

  for (int i = 0; i < peers_.size(); i++) {
    uint64_t peer_id = i + 1;
    if (peer_id == id_) {
      continue;
    }
    transport_->AddPeer(peer_id, peers_[i]);
  }
  Schedule();
}

void Peer::Stop() {
  JLOG_INFO << "stopping";
  peer_store_->Stop();

  if (transport_) {
    transport_->Stop();
    transport_ = nullptr;
  }
  io_service_.stop();
}

void Peer::Propose(const std::string& data, const StatusCallback& cb) {
  if (tid_ != pthread_self()) {
    io_service_.post([this, data, cb]() {
      jraft::ErrNum err = raw_node_->Propose(data);
      if (err != jraft::kOk) {
        cb(Status::IOError("can't propose"));
      } else {
        cb(Status::OK());
      }
      PullReadyEvents();
    });
  } else {
    jraft::ErrNum err = raw_node_->Propose(data);
    if (err != jraft::kOk) {
      cb(Status::IOError("can't propose"));
    } else {
      cb(Status::OK());
    }
    PullReadyEvents();
  }
}

void Peer::Process(MessagePtr msg, const StatusCallback& cb) {
  if (tid_ != pthread_self()) {
    io_service_.post([this, msg, cb]() {
      jraft::ErrNum err = this->raw_node_->Step(*msg);
      if (err != jraft::kOk) {
        cb(Status::IOError("can't process"));
      } else {
        cb(Status::OK());
      }
      PullReadyEvents();
    });
  } else {
    jraft::ErrNum err = this->raw_node_->Step(*msg);
    if (err != jraft::kOk) {
      cb(Status::IOError("can't process"));
    } else {
      cb(Status::OK());
    }
    PullReadyEvents();
  }
}

void Peer::IsIdRemoved(uint64_t id, const std::function<void(bool)>& cb) {
  JLOG_FATAL << "not implemented";
  cb(false);
}

void Peer::ReportUnreachable(uint64_t id) { JLOG_FATAL << "not implemented"; }

void Peer::ReportSnapshot(uint64_t id, SnapshotState state) {
  JLOG_FATAL << "not implemented";
}

bool Peer::PublishEntries(const std::vector<EntryPtr>& ents) {
  for (const auto& ent : ents) {
    if (ent->type() == jraft::kEntryNormal) {
      if (ent->data().empty()) {
        // ignore empty messages
        continue;
      }
      peer_store_->ReadCommit(ent);
    } else if (ent->type() == jraft::kEntryConfChange) {
      jraft::ConfChange cc;
      if (!cc.ParseFromString(ent->data())) {
        JLOG_FATAL << "failed to parse from string";
        continue;
      }
      conf_state_ = raw_node_->ApplyConfChange(cc);

      if (cc.type() == jraft::kConfChangeAddNode) {
        if (!cc.context().empty()) {
          transport_->AddPeer(cc.node_id(), cc.context());
        }
      } else {
        JLOG_FATAL << "not implemented now";
      }
    } else {
      JLOG_FATAL << "invalid entry type";
    }

    // after commit, update appliedIndex
    applied_index_ = ent->index();

    // replay has finished
    if (ent->index() == last_index_) {
      JLOG_INFO << "replay has finished";
    }
  }
  return true;
}

void Peer::EntriesToApply(const std::vector<EntryPtr>& total_ents,
                          std::vector<EntryPtr>* ents_to_apply) {
  if (total_ents.empty()) {
    return;
  }

  uint64_t first = total_ents[0]->index();
  if (first > applied_index_ + 1) {
    JLOG_FATAL << "first index of committed entry " << first
               << " should <= appliedIndex + 1"
               << ", applied_index: " << applied_index_;
  }
  if (applied_index_ - first + 1 < total_ents.size()) {
    ents_to_apply->insert(ents_to_apply->end(),
                          total_ents.begin() + applied_index_ - first + 1,
                          total_ents.end());
  }
}

void Peer::MaybeTriggerSnapshot() {
  if (applied_index_ - snap_index_ <= snap_count_) {
    return;
  }

  JLOG_INFO << "start snapshot [applied_index: " << applied_index_
            << ", snap_index: " << snap_index_
            << ", snap_count: " << snap_count_;
  std::promise<SnapshotDataPtr> promise;
  std::future<SnapshotDataPtr> future = promise.get_future();
  peer_store_->GetSnapshot(std::move(
      [&promise](const SnapshotDataPtr& data) { promise.set_value(data); }));

  future.wait();
  SnapshotDataPtr snapshot_data = future.get();

  SnapshotPtr snap;
  jraft::ErrNum err = storage_->CreateSnapshot(applied_index_, &conf_state_,
                                               *snapshot_data, &snap);
  if (err != jraft::kOk) {
    JLOG_FATAL << "create snapshot error";
  }

  Status status = SaveSnap(*snap);
  if (!status.ok()) {
    JLOG_FATAL << "save snapshot error " << status.ToString();
  }

  uint64_t compact_index = 1;
  if (applied_index_ > kSnapshotCatchupEntriesCount) {
    compact_index = applied_index_ - kSnapshotCatchupEntriesCount;
  }
  err = storage_->Compact(compact_index);
  if (err != jraft::kOk) {
    JLOG_FATAL << "compact error";
  }
  JLOG_INFO << "compacted lot at index " << compact_index;
  snap_index_ = applied_index_;
}

// Replays wal entries into the raft instance.
void Peer::ReplayWal() {
  JLOG_INFO << "replaying wal of node " << id_;

  jraft::Snapshot snap;
  Status status = snapper_->LoadSnap(&snap);
  if (!status.ok()) {
    if (status.IsNotFound()) {
      JLOG_INFO << "snapshot not found for node " << id_;
    } else {
      JLOG_FATAL << "error loading snapshot " << status.ToString();
    }
  } else {
    storage_->ApplySnapshot(snap);
  }

  OpenWal(snap);
  assert(wal_);

  jraft::HardState hs;
  std::vector<jraft::EntryPtr> ents;
  status = wal_->ReadAll(&hs, &ents);
  if (!status.ok()) {
    JLOG_FATAL << "failed to read wal " << status.ToString();
  }

  storage_->set_hard_state(hs);
  // Append to storage so raft starts at the right place in log.
  storage_->Append(ents);

  // Send nil once last_index_ is published so client knows commit channel is
  // current.
  if (!ents.empty()) {
    last_index_ = ents.back()->index();
  } else {
    snap_data_ = snap.data();
  }
}

// Open a wal ready for reading.
void Peer::OpenWal(const jraft::Snapshot& snap) {
  if (!boost::filesystem::exists(wal_dir_)) {
    boost::filesystem::create_directories(wal_dir_);
    Wal::Create(wal_dir_);
  }

  WalSnapshot walsnap;
  walsnap.set_index(snap.metadata().index());
  walsnap.set_term(snap.metadata().term());
  JLOG_INFO << "loading wal at term: " << walsnap.term()
            << ", and index: " << walsnap.index();

  wal_ = std::make_shared<Wal>(wal_dir_, walsnap);
}

void Peer::StartTimer() {
  timer_.expires_from_now(boost::posix_time::millisec(100));
  timer_.async_wait([this](const boost::system::error_code& err) {
    if (err) {
      JLOG_ERROR << "timer waiter error " << err.message();
      return;
    }

    this->StartTimer();
    this->raw_node_->Tick();  // Tick() every 100 ms.
    this->PullReadyEvents();  // whether node has Ready.
  });
}

void Peer::PullReadyEvents() {
  assert(tid_ == pthread_self());

  while (raw_node_->HasReady()) {
    jraft::ReadyPtr rd = raw_node_->GetReady();
    if (!rd->ContainsUpdates()) {  // should be consistent with HasReady()
      JLOG_DEBUG << "ready not contains updates";
      return;
    }

    wal_->Save(rd->hard_state(), rd->entries());

    if (!jraft::IsEmptySnap(rd->mutable_snapshot())) {
      Status status = SaveSnap(*rd->mutable_snapshot());
      if (!status.ok()) {
        JLOG_FATAL << "save snapshot error " << status.ToString();
      }
      storage_->ApplySnapshot(*rd->mutable_snapshot());
      PublishSnap(*rd->mutable_snapshot());
    }

    if (!rd->entries().empty()) {
      storage_->Append(rd->entries());
    }

    if (!rd->messages().empty()) {
      transport_->Send(rd->messages());
    }

    if (!rd->committed_entries().empty()) {
      std::vector<EntryPtr> ents;
      EntriesToApply(rd->committed_entries(), &ents);
      if (!ents.empty()) {
        PublishEntries(ents);
      }
    }

    MaybeTriggerSnapshot();
    raw_node_->Advance(rd);
  }
}

Status Peer::SaveSnap(const jraft::Snapshot& snap) {
  // Must save the Snapshot index to the wal before saving the snapshot to
  // maintain the invariant that we only Open the wal
  // at previously-saved indexes.
  WalSnapshot wal_snap;
  wal_snap.set_index(snap.metadata().index());
  wal_snap.set_term(snap.metadata().term());
  Status status = wal_->SaveSnapshot(wal_snap);
  if (!status.ok()) {
    return status;
  }

  status = snapper_->SaveSnap(snap);
  if (!status.ok()) {
    JLOG_FATAL << "save snapshot error " << status.ToString();
  }
  return status;
}

void Peer::PublishSnap(const jraft::Snapshot& snap) {
  if (jraft::IsEmptySnap(snap)) {
    return;
  }

  JLOG_DEBUG << "publishing snapshot at index " << snap_index_;

  if (snap.metadata().index() <= applied_index_) {
    JLOG_FATAL << "snapshot index " << snap.metadata().index()
               << " should > applied_index_ " << applied_index_ << " + 1";
  }

  // Trigger to load snapshot.
  SnapshotPtr snapshot = std::make_shared<jraft::Snapshot>();
  snapshot->mutable_metadata()->CopyFrom(snap.metadata());
  SnapshotDataPtr data = std::make_shared<std::string>(snap.data());

  conf_state_ = snapshot->metadata().conf_state();
  snap_index_ = snapshot->metadata().index();
  applied_index_ = snapshot->metadata().index();

  peer_store_->RecoverFromSnapshot(
      *data, [snapshot, this](const Status& status) {
        if (!status.ok()) {
          JLOG_FATAL << "recover from snapshot error " << status.ToString();
        }
        JLOG_INFO << "finished publishing snapshot at index " << snap_index_;
      });
}

void Peer::Schedule() {
  tid_ = pthread_self();

  auto snap = std::make_shared<jraft::Snapshot>();
  jraft::ErrNum err_num = storage_->GetSnapshot(snap);
  if (err_num != jraft::kOk) {
    JLOG_FATAL << "can't get snapshot";
  }

  conf_state_ = snap->metadata().conf_state();
  snap_index_ = snap->metadata().index();
  applied_index_ = snap->metadata().index();

  peer_store_ = std::make_shared<PeerStore>(this, snap_data_, port_);
  std::promise<pthread_t> promise;
  std::future<pthread_t> future = promise.get_future();
  peer_store_->Start(promise);
  future.wait();
  pthread_t id = future.get();
  JLOG_INFO << "server start " << id;

  StartTimer();
  io_service_.run();
}

}  // namespace jkv
