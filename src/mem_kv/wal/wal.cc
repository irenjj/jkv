// Copyright (c) renjj - All Rights Reserved
#include "mem_kv/wal/wal.h"

#include <jraft/common/util.h>
#include <jrpc/base/logging/logging.h>

#include <boost/filesystem.hpp>
#include <cinttypes>
#include <sstream>

#include "mem_kv/common/status.h"
#include "mem_kv/common/util.h"
#include "mem_kv/wal/wal_file.h"
#include "mem_kv/wal/wal_record.h"

namespace jkv {

Wal::Wal() : dir_(), start_(), last_index_(0), hard_state_(), files_() {}

Wal::Wal(const std::string& dir)
    : dir_(dir), start_(), last_index_(0), hard_state_(), files_() {}

Wal::Wal(const std::string& dir, const WalSnapshot& snap)
    : dir_(dir), start_(), last_index_(0), hard_state_(), files_() {
  std::vector<std::string> names;
  GetWalNames(dir_, &names);
  if (names.empty()) {
    JLOG_FATAL << "wal not found";
  }

  uint64_t name_index;
  if (!SearchIndex(names, snap.index(), &name_index)) {
    JLOG_FATAL << "wal not found";
  }

  std::vector<std::string> check_names(names.begin() + name_index, names.end());
  if (!IsValidSeq(check_names)) {
    JLOG_FATAL << "invalid wal seq";
  }

  for (const auto& name : names) {
    uint64_t seq;
    uint64_t index;
    if (!ParseWalName(name, &seq, &index)) {
      JLOG_FATAL << "invalid wal name " << name;
    }

    boost::filesystem::path path = boost::filesystem::path(dir_) / name;
    std::shared_ptr<WalFile> file(new WalFile(path.string().c_str(), seq));
    files_.push_back(file);
  }

  memcpy(&start_, &snap, sizeof(snap));
}

void Wal::Create(const std::string& dir) {
  boost::filesystem::path wal_file_path =
      boost::filesystem::path(dir) / WalFileName(0, 0);
  std::string tmp_path = wal_file_path.string() + ".tmp";

  if (boost::filesystem::exists(tmp_path)) {
    boost::filesystem::remove(tmp_path);
  }

  {
    std::unique_ptr<WalFile> wal_file(new WalFile(tmp_path.c_str(), 0));
    WalSnapshot snap;
    snap.set_index(0);
    snap.set_term(0);
    std::string str;
    if (!snap.SerializeToString(&str)) {
      JLOG_FATAL << "failed to serialize to str";
      return;
    }
    wal_file->Append(kWalSnapshotType, (uint8_t*)str.data(), str.size());
    wal_file->Flush();
  }

  boost::filesystem::rename(tmp_path, wal_file_path);
}

// after ReadAll(), the Wal will be ready for appending new records.
Status Wal::ReadAll(jraft::HardState* hs, std::vector<EntryPtr>* ents) {
  std::vector<char> data;
  for (auto& file : files_) {
    data.clear();
    file->Read(&data);
    size_t offset = 0;
    bool match_snap = false;

    while (offset < data.size()) {
      size_t left = data.size() - offset;
      size_t record_begin_offset = offset;

      if (left < sizeof(WalRecord)) {
        file->Truncate(record_begin_offset);
        JLOG_WARN << "invalid record len " << left;
        break;
      }

      WalRecord record;
      memcpy(&record, data.data() + offset, sizeof(record));

      left -= sizeof(record);
      offset += sizeof(record);

      if (record.type() == kWalInvalidType) {
        break;
      }

      uint32_t record_data_len = record.len();
      if (left < record_data_len) {
        file->Truncate(record_begin_offset);
        JLOG_WARN << "invalid record data len " << left << ", "
                  << record_data_len;
        break;
      }

      char* data_ptr = data.data() + offset;
      uint32_t crc = ComputeCrc32(data_ptr, record_data_len);

      left -= record_data_len;
      offset += record_data_len;

      if (record.crc() != 0 && record.crc() != crc) {
        file->Truncate(record_begin_offset);
        JLOG_WARN << "invalid record crc " << record.crc() << ", " << crc;
        break;
      }

      HandleRecordWalRecord(record.type(), data_ptr, record_data_len,
                            &match_snap, hs, ents);

      if (record.type() == kWalSnapshotType) {
        match_snap = true;
      }
    }

    if (!match_snap) {
      JLOG_FATAL << "wal: snapshot not found";
    }
  }

  return Status::OK();
}

Status Wal::Save(const jraft::HardState& hs,
                 const std::vector<EntryPtr>& ents) {
  // shortcut, do not call Flush().
  if (jraft::IsEmptyHardState(hs) && ents.empty()) {
    return Status::OK();
  }

  bool must_flush = IsMustFlush(hs, hard_state_, ents.size());

  Status status;

  for (const auto& entry : ents) {
    status = SaveEntry(*entry);
    if (!status.ok()) {
      return status;
    }
  }

  status = SaveHardState(hs);
  if (!status.ok()) {
    return status;
  }

  if (files_.back()->file_size() < SegmentSizeBytes) {
    if (must_flush) {
      files_.back()->Flush();
    }
    return Status::OK();
  }

  return Cut();
}

Status Wal::SaveSnapshot(const WalSnapshot& snap) {
  std::string str;
  if (!snap.SerializeToString(&str)) {
    JLOG_FATAL << "failed to serialize to str";
    return Status::NotSupported("failed to serialize to str");
  }
  files_.back()->Append(kWalSnapshotType, (uint8_t*)str.data(), str.size());
  if (last_index_ < snap.index()) {
    last_index_ = snap.index();
  }
  files_.back()->Flush();
  return Status::OK();
}

Status Wal::SaveEntry(const jraft::Entry& entry) {
  std::string str;
  if (!entry.SerializeToString(&str)) {
    JLOG_FATAL << "failed to serialize";
    return Status::NotSupported("failed to serialize");
  }
  files_.back()->Append(kWalEntryType, (uint8_t*)str.data(), str.size());
  last_index_ = entry.index();
  return Status::OK();
}

Status Wal::SaveHardState(const jraft::HardState& hs) {
  if (jraft::IsEmptyHardState(hs)) {
    return Status::OK();
  }

  hard_state_ = hs;

  std::string str;
  if (!hs.SerializeToString(&str)) {
    JLOG_FATAL << "failed to serialize";
    return Status::NotSupported("failed to serialize");
  }
  files_.back()->Append(kWalStateType, (uint8_t*)str.data(), str.size());
  return Status::OK();
}

Status Wal::Cut() {
  files_.back()->Flush();
  return Status::OK();
}

Status Wal::ReleaseTo(uint64_t index) { return Status::OK(); }

void Wal::GetWalNames(const std::string& dir, std::vector<std::string>* names) {
  boost::filesystem::directory_iterator end;
  for (boost::filesystem::directory_iterator it(dir); it != end; it++) {
    boost::filesystem::path filename = (*it).path().filename();
    boost::filesystem::path extension = filename.extension();
    if (extension != ".wal") {
      JLOG_WARN << "extension: " << extension.string() << " is not .wal";
      continue;
    }
    names->push_back(filename.string());
  }
  std::sort(names->begin(), names->end(), std::less<std::string>());
}

bool Wal::ParseWalName(const std::string& name, uint64_t* seq,
                       uint64_t* index) {
  *seq = 0;
  *index = 0;

  boost::filesystem::path path(name);
  if (path.extension() != ".wal") {
    return false;
  }

  std::string filename = name.substr(0, name.size() - 4);  // trim ".wal"
  size_t pos = filename.find('-');
  if (pos == std::string::npos) {
    return false;
  }

  try {
    {
      std::string str = filename.substr(0, pos);
      std::stringstream ss;
      ss << std::hex << str;
      ss >> *seq;
    }

    {
      if (pos == filename.size() - 1) {
        return false;
      }
      std::string str = filename.substr(pos + 1, filename.size() - pos - 1);
      std::stringstream ss;
      ss << std::hex << str;
      ss >> *index;
    }
  } catch (...) {
    return false;
  }
  return true;
}

bool Wal::IsValidSeq(const std::vector<std::string>& names) {
  uint64_t last_seq = 0;
  for (const auto& name : names) {
    uint64_t cur_seq;
    uint64_t i;
    if (!ParseWalName(name, &cur_seq, &i)) {
      JLOG_FATAL << "parse correct name should never fail " << name.c_str();
    }

    if (last_seq != 0 && last_seq != cur_seq - 1) {
      return false;
    }
    last_seq = cur_seq;
  }

  return true;
}

// SearchIndex returns the last array index of names whose raft index section is
// equal to or smaller than the given index. The given names MUST be sorted.
bool Wal::SearchIndex(const std::vector<std::string>& names, uint64_t index,
                      uint64_t* name_index) {
  for (int i = names.size() - 1; i >= 0; i--) {
    const std::string& name = names[i];
    uint64_t seq;
    uint64_t cur_index;
    if (!ParseWalName(name, &seq, &cur_index)) {
      JLOG_FATAL << "invalid wal name " << name.c_str();
    }

    if (index >= cur_index) {
      *name_index = i;
      return true;
    }
  }

  *name_index = -1;
  return false;
}

std::string Wal::WalFileName(uint64_t seq, uint64_t index) {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%016" PRIx64 "-%016" PRIx64 ".wal", seq,
           index);
  return buffer;
}

void Wal::HandleRecordWalRecord(WalType type, const char* data, size_t data_len,
                                bool* match_snap, jraft::HardState* hs,
                                std::vector<EntryPtr>* ents) {
  if (type == kWalEntryType) {
    EntryPtr ent;
    if (!ent->ParseFromArray((const void*)data, data_len)) {
      JLOG_FATAL << "failed to parse from array";
    }

    if (ent->index() > start_.index()) {
      ents->resize(ent->index() - start_.index() - 1);
      ents->push_back(ent);
    }

    last_index_ = ent->index();
  } else if (type == kWalStateType) {
    if (!hs->ParseFromArray((const void*)data, data_len)) {
      JLOG_FATAL << "failed to parse from array";
    }
  } else if (type == kWalSnapshotType) {
    WalSnapshot snap;
    if (!snap.ParseFromArray((const void*)data, data_len)) {
      JLOG_FATAL << "failed to parse from array";
      return;
    }

    if (snap.index() == start_.index()) {
      if (snap.term() != start_.term()) {
        JLOG_FATAL << "wal: snapshot mismatch";
      }
      *match_snap = true;
    }
  } else if (type == kWalCrcType) {
    JLOG_FATAL << "crc type not implemented";
  } else {
    JLOG_FATAL << "invalid record type " << type;
  }
}

}  // namespace jkv
