include(FindProtobuf)
file(GLOB PROTO_LIST ${CMAKE_SOURCE_DIR}/pb/*.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HEADERS ${PROTO_LIST})

set(COMMLIBS
    pthread
    protobuf
    gflags
    event
    libevent.a
    pthread
    jraft.a
    jrpc.a
    jkv
    numa)

aux_source_directory(./server SERVER_SRCS)

find_package(Threads)

add_executable(kv_server ${SERVER_SRCS} ${PROTO_SRCS})
target_link_libraries(kv_server -g
    ${COMMLIBS} ${LIBS} ${CMAKE_THREAD_LIBS_INIT})
