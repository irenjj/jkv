add_executable(snapper_test snapper_test.cc)
target_link_libraries(snapper_test gtest gtest_main gmock gmock_main
    -Wl,--no-whole-archive ${COMMLIBS} ${LIBS})
