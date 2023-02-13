// Copyright (c) renjj - All Rights Reserved
#include "mem_kv/snapper/snapper.h"

#include <jrpc/base/logging/logging.h>

#include <boost/filesystem.hpp>
#include <cinttypes>

#include "mem_kv/common/util.h"

namespace jkv {

struct SnapshotRecord {
  uint32_t data_len;
  uint32_t crc32;
  char data[0]{};

  SnapshotRecord() : data_len(0), crc32(0) {}
  ~SnapshotRecord() = default;
};

Status Snapper::LoadSnap(jraft::Snapshot* snap) {
  std::vector<std::string> names;
  GetSnapNames(&names);

  for (const auto& name : names) {
    Status status = LoadSnap(snap, name);
    if (status.ok()) {
      return Status::OK();
    }
  }

  return Status::NotFound("snapshot not found");
}

Status Snapper::SaveSnap(const jraft::Snapshot& snap) {
  Status status;
  std::string str;
  if (!snap.SerializeToString(&str)) {
    JLOG_FATAL << "failed to serialize to str";
  }

  auto record =
      static_cast<SnapshotRecord*>(malloc(str.size() + sizeof(SnapshotRecord)));
  record->data_len = str.size();
  record->crc32 = ComputeCrc32(str.data(), str.size());
  memcpy(record->data, str.data(), str.size());

  char save_path[128];
  snprintf(save_path, sizeof(save_path), "%s/%s", dir_.c_str(),
           SnapName(snap.metadata().term(), snap.metadata().index()).c_str());

  FILE* fp = fopen(save_path, "w");
  if (!fp) {
    free(record);
    JLOG_ERROR << "can't open fp";
    return Status::IOError(strerror(errno));
  }

  size_t bytes = sizeof(SnapshotRecord) + record->data_len;
  if (fwrite((void*)record, 1, bytes, fp) != bytes) {
    JLOG_ERROR << "write record error";
    status = Status::IOError(strerror(errno));
  }
  free(record);
  fclose(fp);

  return status;
}

std::string Snapper::SnapName(uint64_t term, uint64_t index) {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%016" PRIx64 "-%016" PRIx64 ".snap", term,
           index);
  return buffer;
}

void Snapper::GetSnapNames(std::vector<std::string>* names) {
  boost::filesystem::directory_iterator end;
  for (boost::filesystem::directory_iterator it(dir_); it != end; it++) {
    boost::filesystem::path filename = it->path().filename();
    boost::filesystem::path extension = filename.extension();
    if (extension != ".snap") {
      continue;
    }
    names->push_back(filename.string());
  }
  std::sort(names->begin(), names->end(), std::greater<std::string>());
}

Status Snapper::LoadSnap(jraft::Snapshot* snap, const std::string& filename) {
  SnapshotRecord snap_hdr;
  std::vector<char> data;
  boost::filesystem::path path = boost::filesystem::path(dir_) / filename;
  FILE* fp = fopen(path.c_str(), "r");
  if (!fp) {
    goto invalid_snap;
  }

  if (fread(&snap_hdr, 1, sizeof(SnapshotRecord), fp) !=
      sizeof(SnapshotRecord)) {
    goto invalid_snap;
  }

  if (snap_hdr.data_len == 0 || snap_hdr.crc32 == 0) {
    goto invalid_snap;
  }

  data.resize(snap_hdr.data_len);
  if (fread(data.data(), 1, snap_hdr.data_len, fp) != snap_hdr.data_len) {
    goto invalid_snap;
  }

  fclose(fp);
  fp = nullptr;
  if (ComputeCrc32(data.data(), data.size()) != snap_hdr.crc32) {
    goto invalid_snap;
  }

  try {
    if (!snap->ParseFromArray((const void*)data.data(), data.size())) {
      JLOG_FATAL << "failed to parse";
    }
    return Status::OK();
  } catch (std::exception& e) {
    goto invalid_snap;
  }

invalid_snap:
  if (fp) {
    fclose(fp);
  }
  JLOG_INFO << "broken snapshot " << path.string().c_str();
  boost::filesystem::rename(path, path.string() + ".broken");
  return Status::IOError("unexpected empty snapshot");
}

}  // namespace jkv
