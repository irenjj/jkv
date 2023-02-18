include(FindProtobuf)
file(GLOB PROTO_LIST ${CMAKE_SOURCE_DIR}/pb/*.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HEADERS ${PROTO_LIST})

aux_source_directory(common MEMKV_SRCS)
aux_source_directory(peer MEMKV_SRCS)
aux_source_directory(snapper MEMKV_SRCS)
aux_source_directory(transport MEMKV_SRCS)
aux_source_directory(wal MEMKV_SRCS)

add_library(mem_kv STATIC ${MEMKV_SRCS} ${PROTO_SRCS})
target_link_libraries(mem_kv jraft)
