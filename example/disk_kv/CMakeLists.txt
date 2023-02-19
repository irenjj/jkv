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
    numa
    leveldb
    jraft.a
    jrpc.a
    jkv)

aux_source_directory(./server SERVER_SRCS)

find_package(Threads)

add_executable(kv_server ${SERVER_SRCS} ${PROTO_SRCS})
target_link_libraries(kv_server -g
    ${COMMLIBS} ${LIBS} ${CMAKE_THREAD_LIBS_INIT})

aux_source_directory(./client CLIENT_SRCS)

add_executable(kv_client ${CLIENT_SRCS})
target_link_libraries(kv_client -g
    ${COMMLIBS} ${LIBS} ${CMAKE_THREAD_LIBS_INIT})
