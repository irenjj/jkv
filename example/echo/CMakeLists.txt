include(FindProtobuf)
file(GLOB PROTO_LIST ${CMAKE_SOURCE_DIR}/pb/*.proto)
protobuf_generate_cpp(PROTO_SRCS PROTO_HEADERS ${PROTO_LIST})

aux_source_directory(server SERVER_SRCS)
add_executable(echo_server ${SERVER_SRCS} ${PROTO_SRCS})
target_link_libraries(echo_server -g
    ${COMMLIBS} ${LIBS} ${CMAKE_THREAD_LIBS_INIT})

aux_source_directory(client CLIENT_SRCS)
add_executable(echo_client ${CLIENT_SRCS})
target_link_libraries(echo_client -g
    ${COMMLIBS} ${LIBS} ${CMAKE_THREAD_LIBS_INIT})
