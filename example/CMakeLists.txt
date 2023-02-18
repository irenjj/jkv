set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g -fprofile-arcs \
                      -ftest-coverage -Wall -std=c++11")

#link_directories(${CMAKE_SOURCE_DIR}/build/src/disk_kv)

add_subdirectory(disk_kv)
