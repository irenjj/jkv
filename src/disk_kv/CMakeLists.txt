include(FindProtobuf)
file(GLOB PROTO_LIST ${CMAKE_SOURCE_DIR}/pb/*.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HEADERS ${PROTO_LIST})

aux_source_directory(common JKV_SRCS)
aux_source_directory(peer JKV_SRCS)
aux_source_directory(service JKV_SRCS)

add_library(jkv ${JKV_SRCS} ${PROTO_SRCS})
