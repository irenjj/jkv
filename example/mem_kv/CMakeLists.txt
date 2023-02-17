include(FindProtobuf)
file(GLOB PROTO_LIST ${CMAKE_SOURCE_DIR}/pb/*.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HEADERS ${PROTO_LIST})

add_executable(kv_server ${PROTO_SRCS} main.cc)
target_link_libraries(kv_server
                      -Wl,--no-whole-archive ${COMMLIBS} ${LIBS})
