// Copyright (c) renjj - All Rights Reserved
#pragma once

namespace jkv {

enum WalType {
  kWalInvalidType = 0,
  kWalEntryType = 1,
  kWalStateType = 2,
  kWalCrcType = 3,
  kWalSnapshotType = 4,
};

static const int SegmentSizeBytes = 64 * 1024 * 1024;  // 64 MB

}  // namespace jkv
