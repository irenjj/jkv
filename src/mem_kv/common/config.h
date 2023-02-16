// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <cstdint>
#include <functional>

namespace jkv {

class Status;

constexpr uint64_t kDefaultSnapCount = 100000;
constexpr uint64_t kSnapshotCatchupEntriesCount = 100000;

using StatusCallback = std::function<void(const Status&)>;
using SnapshotDataPtr = std::shared_ptr<std::string>;
using GetSnapshotCallback = std::function<void(SnapshotDataPtr)>;

enum SnapshotState {
  kSnapshotFinished = 1,
  kSnapshotFailed = 2,
};

constexpr uint64_t kReceiveBufferSize = 1024 * 512;

}  // namespace jkv
