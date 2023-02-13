// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <cstddef>
#include <cstdint>

#include "eraft.pb.h"

namespace jkv {

uint32_t ComputeCrc32(const char* data, size_t len);
bool IsMustFlush(const jraft::HardState& hs, const jraft::HardState& prev_hs,
                 size_t ents_num);

}  // namespace jkv
