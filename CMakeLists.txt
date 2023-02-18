cmake_minimum_required(VERSION 3.16)
project(jkv VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-aligned-new")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

link_directories(third-party/jraft/output/lib/)
link_directories(third-party/jrpc/lib/)

include_directories(build/src/)
include_directories(build/src/mem_kv)
include_directories(build/src/disk_kv)
include_directories(src/)
include_directories(${CMAKE_SOURCE_DIR})
include_directories(third-party/jraft/output/include/)
include_directories(third-party/jraft/output/include/jraft/)
include_directories(third-party/jrpc/include/)
include_directories(third-party/jrpc/include/jrpc/)

find_package(Boost REQUIRED COMPONENTS system filesystem)
include_directories(${Boost_INCLUDE_DIRS})
set(LIBS
    Boost::system
    Boost::filesystem)

add_subdirectory(src)
add_subdirectory(example)