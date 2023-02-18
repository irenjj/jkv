// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <jraft/rawnode.h>
#include <jrpc/net/event/event_worker_colony.h>
#include <jrpc/net/rpc/rpc_controller.h>

#include <deque>

#include "disk_kv/common/config.h"
#include "memory_storage.h"
#include "raft_server.pb.h"

namespace jkv {

class PeerHost;

// some address infos of same group of peers on different hosts
struct GroupMember {
  // on which host
  HostId host_id = kNoHost;
  // on which group of the host
  PeerId peer_id = kNoPeer;
  std::string ip;
  uint16_t port = 0;

  GroupMember(HostId host_id, PeerId peer_id, const std::string& ip,
              uint16_t port)
      : host_id(host_id), peer_id(peer_id), ip(ip), port(port) {}
};

struct Proposal {
  uint64_t index;
  uint64_t term;
  KvResp* resp;
  ::google::protobuf::Closure* done;

  Proposal(uint64_t index, uint64_t term, KvResp* resp,
           ::google::protobuf::Closure* done)
      : index(index), term(term), resp(resp), done(done) {}
};

using ProposalPtr = std::shared_ptr<Proposal>;

class Peer {
 public:
  Peer(const PeerOption& peer_opt, PeerHost* host);
  ~Peer();

  void StartTimer();
  void Tick();

  jraft::ErrNum Step(const jraft::Message& msg);
  void ProposeKvOp(const KvReq* req, KvResp* resp,
                   ::google::protobuf::Closure* done);

  bool IsLeader() const { return raw_node_->IsLeader(); }
  uint64_t Term() const { return raw_node_->raft().term(); }

 private:
  void InitGroupMembers(const std::string& members);
  void HandleRaftReady();

  void SendMsgs(const std::vector<jraft::Message>& msgs);
  void SendMsg2Peer(const jraft::Message& msg);
  void HandleSendMsg2Peer(jrpc::RpcController* cntl, RaftMsgResp* cb);

  void ProcessEntries(const std::vector<jraft::EntryPtr>& entries);
  void ProcessEntry(const jraft::EntryPtr& entry);
  void ProcessNormal(const jraft::EntryPtr& entry, const ProposalPtr& p);
  void ProcessConfChange(const jraft::EntryPtr& entry, const ProposalPtr& p);

  ProposalPtr GetProposal(uint64_t index, uint64_t term);
  void CallbackAll();

  uint64_t NextProposalIndex() const {
    return raw_node_->raft().raft_log()->LastIndex() + 1;
  }

  PeerHost* host_;
  GroupMember self_;
  jraft::MemoryStoragePtr mem_storage_;
  jrpc::EventWorker* worker_;
  // all member info of the group
  std::unordered_map<HostId, GroupMember> peers_;
  jraft::RawNode* raw_node_;
  std::deque<ProposalPtr> proposals_;
};

using PeerPtr = std::shared_ptr<Peer>;

}  // namespace jkv
