set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g -fprofile-arcs \
                      -ftest-coverage -Wall -std=c++11")

find_library(GTEST_LIB NAMES gtest)
if (NOT GTEST_LIB)
  message(FATAL_ERROR "Fail to find gtest")
endif()
find_library(GTEST_MAIN_LIB NAMES gtest_main)
if (NOT GTEST_MAIN_LIB)
  message(FATAL_ERROR "Fail to find gtest main")
endif()
find_library(GMOCK_LIB NAMES gmock)
if (NOT GMOCK_LIB)
  message(FATAL_ERROR "Fail to find gmock")
endif()
find_library(GMOCK_MAIN_LIB NAMES gmock_main)
if (NOT GMOCK_MAIN_LIB)
  message(FATAL_ERROR "Fail to find gmock main")
endif()

link_directories(${CMAKE_SOURCE_DIR}/build/src/)

set(COMMLIBS
    pthread
    gtest
    protobuf.a
    jraft
    jrpc.a
    jkv)

add_subdirectory(snapper_test)
add_subdirectory(wal_test)