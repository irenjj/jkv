add_executable(byte_buffer_test byte_buffer_test.cc)
target_link_libraries(byte_buffer_test gtest gtest_main gmock gmock_main
    -Wl,--no-whole-archive ${COMMLIBS} ${LIBS})
