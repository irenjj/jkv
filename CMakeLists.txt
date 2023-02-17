cmake_minimum_required(VERSION 3.16)
project(jkv VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)

link_directories(third-party/jraft/output/lib/)
link_directories(third-party/jrpc/lib/)

include_directories(build/src/)
include_directories(src/)
include_directories(third-party/jraft/output/include/)
include_directories(third-party/jraft/output/include/jraft/)
include_directories(third-party/jrpc/include/)
include_directories(third-party/jrpc/include/jrpc/)

include_directories(${CMAKE_SOURCE_DIR})

find_package(PkgConfig REQUIRED)
pkg_check_modules(dependencies
    glib-2.0>=2.10 REQUIRED
    hiredis REQUIRED)
include_directories(${dependencies_INCLUDE_DIRS})
set(LIBS
    ${dependencies_LIBRARIES})

find_package(Boost REQUIRED COMPONENTS system filesystem)
include_directories(${Boost_INCLUDE_DIRS})
set(LIBS
    ${LIBS}
    Boost::system
    Boost::filesystem)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(example)