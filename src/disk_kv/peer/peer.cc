// Copyright (c) renjj - All Rights Reserved
#include "disk_kv/peer/peer.h"

#include <jraft/ready.h>
#include <jrpc/net/event/event_loop.h>

#include <boost/algorithm/string.hpp>
#include <functional>

#include "disk_kv/peer/peer_host.h"
#include "disk_kv/storage/disk_storage.h"

namespace jkv {

Peer::Peer(const PeerOption& peer_opt, PeerHost* host)
    : host_(host),
      self_(peer_opt.host_id, peer_opt.peer_id, peer_opt.ip, peer_opt.port),
      mem_storage_(peer_opt.storage),
      worker_(peer_opt.worker),
      raw_node_(nullptr),
      kv_db_(nullptr) {
  // initialize members_
  InitGroupMembers(peer_opt.members);

  // construct config
  jraft::Config c;
  c.id = self_.host_id;
  c.storage = peer_opt.storage;
  if (!c.storage) {
    JLOG_FATAL << "storage should constructed at start";
  }
  // TODO: configurations should be passed in manually
  c.election_tick = 8;
  c.heartbeat_tick = 1;
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

  // construct raw node
  jraft::SnapshotPtr snap;
  jraft::ErrNum err = mem_storage_->GetSnapshot(snap);
  if (err != jraft::kOk) {
    JLOG_FATAL << "can't get snapshot from storage";
  }
  jraft::ConfState* conf_state = snap->mutable_metadata()->mutable_conf_state();
  for (const auto& p : peers_) {
    conf_state->add_voters(p.first);
  }
  raw_node_ = new jraft::RawNode(c);
  kv_db_ = new DiskStorage(host_->db_path() + "/" + "peer_" +
                           std::to_string(peer_opt.peer_id));
}

Peer::~Peer() {
  if (raw_node_) {
    delete raw_node_;
  }
  if (kv_db_) {
    delete kv_db_;
  }
}

void Peer::StartTimer() {
  jrpc::TimerCb tick_cb = std::bind(&Peer::Tick, this);
  worker_->loop()->RunEvery(0.1, tick_cb);
}

void Peer::Tick() {
  raw_node_->Tick();
  HandleRaftReady();
}

jraft::ErrNum Peer::Step(const jraft::Message& msg) {
  return raw_node_->Step(msg);
}

void Peer::ProposeKvOp(const KvReq* req, KvResp* resp,
                       ::google::protobuf::Closure* done) {
  std::string context;
  if (!req->SerializeToString(&context)) {
    JLOG_FATAL << "failed to serialize to str";
  }

  uint64_t ni = NextProposalIndex();
  ProposalPtr p = std::make_shared<Proposal>(ni, Term(), resp, done);
  if (IsLeader()) {
    proposals_.push_back(p);
  }
  raw_node_->Propose(context);
}

// members format: host1,peer1,ip1,port1|host2,peer2,ip2,port2|...
void Peer::InitGroupMembers(const std::string& members) {
  std::vector<std::string> strs;
  boost::split(strs, members, boost::is_any_of("|"));
  if (strs.empty()) {
    JLOG_FATAL << "invalid members";
  }

  for (const auto& str : strs) {
    std::vector<std::string> addr_info;
    boost::split(addr_info, str, boost::is_any_of(","));
    if (addr_info.size() != 4) {
      JLOG_FATAL << "invalid member";
    }
    if (peers_.find(atoi(addr_info[0].c_str())) != peers_.end()) {
      JLOG_FATAL << "same host in one cluster";
    }
    GroupMember group_member(atoi(addr_info[0].c_str()),
                             atoi(addr_info[1].c_str()), addr_info[2],
                             atoi(addr_info[3].c_str()));
    peers_.insert({group_member.host_id, group_member});
  }
}

void Peer::HandleRaftReady() {
  if (!raw_node_->HasReady()) {
    return;
  }

  jraft::ReadyPtr rd = raw_node_->GetReady();

  if (!rd->entries().empty()) {
    raw_node_->mutable_raft()->mutable_storage()->Append(rd->entries());
  }

  if (!rd->messages().empty()) {
    SendMsgs(rd->messages());
  }

  if (!rd->committed_entries().empty()) {
    ProcessEntries(rd->committed_entries());
  }

  raw_node_->Advance(rd);
}

void Peer::SendMsgs(const std::vector<jraft::Message>& msgs) {
  for (const auto& msg : msgs) {
    SendMsg2Peer(msg);
  }
}

void Peer::SendMsg2Peer(const jraft::Message& msg) {
  RaftMsgReq raft_msg;
  GroupMember to_member = peers_.find(msg.to())->second;
  raft_msg.mutable_msg()->CopyFrom(msg);
  raft_msg.mutable_from_peer()->set_host_id(self_.host_id);
  raft_msg.mutable_from_peer()->set_peer_id(self_.peer_id);
  raft_msg.mutable_to_peer()->set_host_id(to_member.host_id);
  raft_msg.mutable_to_peer()->set_peer_id(to_member.peer_id);

  jrpc::SockAddress sock_addr(to_member.ip, to_member.port);
  std::shared_ptr<jrpc::RpcClient> client =
      host_->GetClient(msg.to(), worker_, sock_addr, 300);
  RaftMsgService_Stub stub(client.get());
  auto cntl = new jrpc::RpcController(client->conn());
  auto raft_msg_cb = new RaftMsgResp;
  google::protobuf::Closure* done = google::protobuf::NewCallback(
      this, &Peer::HandleSendMsg2Peer, cntl, raft_msg_cb);
  stub.HandleRaftMsg(cntl, &raft_msg, raft_msg_cb, done);
}

void Peer::HandleSendMsg2Peer(jrpc::RpcController* cntl, RaftMsgResp* resp) {
  std::unique_ptr<jrpc::RpcController> cntl_guard(cntl);
  std::unique_ptr<RaftMsgResp> resp_guard(resp);
}

void Peer::ProcessEntries(const std::vector<jraft::EntryPtr>& entries) {
  for (const auto& ent : entries) {
    ProcessEntry(ent);
  }
}

void Peer::ProcessEntry(const jraft::EntryPtr& entry) {
  ProposalPtr p = nullptr;
  if (IsLeader()) {
    p = GetProposal(entry->index(), entry->term());
  } else {
    CallbackAll();
  }

  if (entry->type() == jraft::kEntryNormal) {
    ProcessNormal(entry, p);
  } else {
    ProcessConfChange(entry, p);
  }
}

void Peer::ProcessNormal(const jraft::EntryPtr& entry, const ProposalPtr& p) {
  KvReq req;
  if (!req.ParseFromString(entry->data())) {
    JLOG_FATAL << "failed to parse from str";
  }
  CmdType type = req.cmd_type();
  if (type == kInvalid) {
    JLOG_DEBUG << "no op";
    return;
  } else if (type == kGet) {
    const GetReq& get_req = req.get_req();
    std::string val = kv_db_->Get(get_req.key());
    if (p != nullptr) {
      GetResp* get_resp = p->resp->mutable_get_resp();
      get_resp->set_value(val);
      p->done->Run();
    }
  } else if (type == kPut) {
    const PutReq& put_req = req.put_req();
    kv_db_->Put(put_req.key(), put_req.value());
    if (p != nullptr) {
      p->done->Run();
    }
  } else if (type == kDel) {
    const DelReq& del_req = req.del_req();
    kv_db_->Del(del_req.key());
    if (p != nullptr) {
      p->done->Run();
    }
  } else {
    JLOG_FATAL << "should not come here.";
  }
}

void Peer::ProcessConfChange(const jraft::EntryPtr& entry,
                             const ProposalPtr& p) {
  // TODO: finish it
}

ProposalPtr Peer::GetProposal(uint64_t index, uint64_t term) {
  while (!proposals_.empty()) {
    ProposalPtr p = proposals_.front();

    if (term == p->term) {
      if (index == p->index) {
        proposals_.pop_front();
        return p;
      }
      if (index < p->index) {
        return nullptr;
      }
      if (index > p->index) {
        proposals_.pop_front();
        p->resp->set_rejected(true);
        p->done->Run();
      }
    } else {
      if (term < p->term) {
        return nullptr;
      }

      if (term > p->term) {
        proposals_.pop_front();
        p->resp->set_rejected(true);
        p->done->Run();
      }
    }
  }

  return nullptr;
}

void Peer::CallbackAll() {
  for (const auto& p : proposals_) {
    p->resp->set_rejected(true);
    p->done->Run();
  }
  proposals_.clear();
}

}  // namespace jkv
