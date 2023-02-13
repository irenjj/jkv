include(FindProtobuf)
file(GLOB PROTO_LIST ${CMAKE_SOURCE_DIR}/pb/*.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HEADERS ${PROTO_LIST})

aux_source_directory(mem_kv/common JKV_SRCS)
aux_source_directory(mem_kv/wal JKV_SRCS)

add_library(jkv STATIC ${JKV_SRCS} ${PROTO_SRCS})
