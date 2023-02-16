// Copyright (c) renjj - All Rights Reserved
#include <boost/filesystem.hpp>
#include <gtest/gtest.h>

#include "mem_kv/common/status.h"
#include "mem_kv/snapper/snapper.h"

std::string GetTmpSnapshotDir() {
  char buffer[128];
  snprintf(buffer, sizeof(buffer), "_test_snapshot/%d_%d", (int)time(nullptr), getpid());
  return buffer;
}

jraft::Snapshot GetTestSnap() {
  jraft::Snapshot snap;
  jraft::SnapshotMetadata* meta = snap.mutable_metadata();
  meta->set_index(1);
  meta->set_term(1);
  meta->mutable_conf_state()->mutable_voters()->Add(1);
  meta->mutable_conf_state()->mutable_voters()->Add(2);
  meta->mutable_conf_state()->mutable_voters()->Add(3);

  snap.set_data("test");

  return snap;
}

bool SnapEq(const jraft::Snapshot& s1, const jraft::Snapshot& s2) {
  if (s1.data() != s2.data()) {
    return false;
  }
  jraft::SnapshotMetadata m1 = s1.metadata();
  jraft::SnapshotMetadata m2 = s2.metadata();
  if (m1.index() != m2.index()) {
    return false;
  }
  if (m1.term() != m2.term()) {
    return false;
  }
  if (m1.conf_state().voters().size() != m2.conf_state().voters().size()) {
    return false;
  }
  for (int i = 0; i < m1.conf_state().voters().size(); i++) {
    if (m1.conf_state().voters(i) != m2.conf_state().voters(i)) {
      return false;
    }
  }

  return true;
}

TEST(SnapperTest, SaveAndLoad) {
  std::string dir = GetTmpSnapshotDir();
  boost::filesystem::create_directories(dir);

  char tmp[256];
  snprintf(tmp, sizeof(tmp), "%s/%s", dir.c_str(), jkv::Snapper::SnapName(0xFFFF, 0xFFFF).c_str());
  FILE* fp = fopen(tmp, "w");
  fwrite("somedata", 1, 5, fp);
  fclose(fp);

  jkv::Snapper snapper(dir);
  jraft::Snapshot s = GetTestSnap();
  jkv::Status status = snapper.SaveSnap(s);
  ASSERT_TRUE(status.ok());

  jraft::Snapshot s2;
  status = snapper.LoadSnap(&s2);
  ASSERT_TRUE(status.ok());
  ASSERT_TRUE(SnapEq(s, s2));
  std::string broken = std::string(tmp) + ".broken";
  ASSERT_TRUE(boost::filesystem::exists(broken));
}

int main(int argc, char* argv[]) {
  boost::system::error_code code;
  boost::filesystem::create_directories("_test_snapshot");

  testing::InitGoogleTest(&argc, argv);
  int ret = RUN_ALL_TESTS();

  boost::filesystem::remove_all("_test_snapshot", code);
  return ret;
}
