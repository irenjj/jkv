add_executable(wal_test wal_test.cc)
target_link_libraries(wal_test gtest gtest_main gmock gmock_main
    -Wl,--no-whole-archive ${COMMLIBS} ${LIBS})
