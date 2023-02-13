// Copyright (c) renjj - All Rights Reserved
#include "mem_kv/wal/wal_file.h"

#include <jrpc/base/logging/logging.h>
#include <unistd.h>

#include "mem_kv/common/util.h"
#include "mem_kv/wal/wal_record.h"

namespace jkv {

WalFile::WalFile(const char* path, uint64_t seq) : seq_(seq), file_size_(0) {
  fp_ = fopen(path, "a+");
  if (!fp_) {
    JLOG_FATAL << "fopen error " << strerror(errno);
  }

  file_size_ += ftell(fp_);
  if (file_size_ == -1) {
    JLOG_FATAL << "ftell error " << strerror(errno);
  }

  if (fseek(fp_, 0L, SEEK_SET) == -1) {
    JLOG_FATAL << "fseek error " << strerror(errno);
  }
}

WalFile::~WalFile() { fclose(fp_); }

void WalFile::Append(WalType type, const uint8_t* data, size_t len) {
  WalRecord record(type, ComputeCrc32((char*)data, len));
  record.set_len(len);
  auto ptr = (uint8_t*)&record;
  data_buffer_.insert(data_buffer_.end(), ptr, ptr + sizeof(record));
  data_buffer_.insert(data_buffer_.end(), data, data + len);
}

void WalFile::Truncate(size_t offset) {
  if (ftruncate(fileno(fp_), offset) != 0) {
    JLOG_FATAL << "ftruncate error " << strerror(errno);
  }

  if (fseek(fp_, offset, SEEK_SET) == -1) {
    JLOG_FATAL << "fseek error " << strerror(errno);
  }

  file_size_ = offset;
  data_buffer_.clear();
}

void WalFile::Flush() {
  if (data_buffer_.empty()) {
    JLOG_INFO << "data_buffer_ empty";
    return;
  }

  size_t bytes = fwrite(data_buffer_.data(), 1, data_buffer_.size(), fp_);
  if (bytes != data_buffer_.size()) {
    JLOG_FATAL << "fwrite error " << strerror(errno);
  }

  file_size_ += data_buffer_.size();
  data_buffer_.clear();
}

void WalFile::Read(std::vector<char>* out) {
  char buffer[1024];
  while (true) {
    size_t bytes = fread(buffer, 1, sizeof(buffer), fp_);
    if (bytes == 0) {
      break;
    }

    out->insert(out->end(), buffer, buffer + bytes);
  }

  fseek(fp_, 0L, SEEK_END);
}

}  // namespace jkv
