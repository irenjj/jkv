set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g -fprofile-arcs \
                      -ftest-coverage -Wall -std=c++11")

link_directories(${CMAKE_SOURCE_DIR}/build/src/)

set(COMMLIBS
    pthread
    gtest
    protobuf
    jraft
    jrpc
    jkv)

add_subdirectory(mem_kv)
