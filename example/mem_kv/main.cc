// Copyright (c) renjj - All Rights Reserved
#include <cstdint>

#include <jrpc/base/logging/logging.h>
#include <glib.h>

#include "mem_kv/peer/peer.h"

uint64_t g_id = 0;
const char* g_cluster = nullptr;
uint16_t g_port = 0;

int main(int argc, char** argv) {
  jbase::Logger::setLogLevel("debug");

  GOptionEntry entries[] = {
      {"id", 'i', 0, G_OPTION_ARG_INT64, &g_id, "node id", nullptr},
      {"cluster", 'c', 0, G_OPTION_ARG_STRING, &g_cluster, "comma separated cluster peers", nullptr},
      {"port", 'p', 0, G_OPTION_ARG_INT, &g_port, "key-value server port", nullptr},
      {nullptr}
  };

  GError* err = nullptr;
  GOptionContext* context = g_option_context_new("usage");
  g_option_context_add_main_entries(context, entries, nullptr);
  if (!g_option_context_parse(context, &argc, &argv, &err)) {
    JLOG_FATAL << "invalid args";
  }
  JLOG_INFO << "id: " << g_id << ", port: " << g_port << ", cluster: " << g_cluster;

  if (g_id == 0 || g_port == 0) {
    JLOG_FATAL << "invalid! id: " << g_id << ", port: " << g_port;
  }

  std::shared_ptr<jkv::Peer> peer = std::make_shared<jkv::Peer>(g_id, g_port, g_cluster);
  peer->Start();

  g_option_context_free(context);

  return 0;
}
