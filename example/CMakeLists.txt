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

find_package(Boost REQUIRED COMPONENTS system filesystem)
include_directories(${Boost_INCLUDE_DIRS})
set(LIBS
    Boost::system
    Boost::filesystem)

add_subdirectory(echo)
