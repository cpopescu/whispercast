project (EMOTICON)


SET(EXECUTABLE rpc_server_first_hand_written)
ADD_EXECUTABLE(${EXECUTABLE}  types.cc
                              server/services_impl.cc
                              server/invokers.cc
                              server/server.cc)
ADD_DEPENDENCIES(${EXECUTABLE}       net_rpc)
TARGET_LINK_LIBRARIES(${EXECUTABLE}  net_rpc)
#ADD_TEST(${EXECUTABLE}              ${EXECUTABLE})


SET(EXECUTABLE rpc_client_first_hand_written)
ADD_EXECUTABLE(${EXECUTABLE}  types.cc
                              client/wrappers.cc
                              client/client.cc)
ADD_DEPENDENCIES(${EXECUTABLE}       net_rpc)
TARGET_LINK_LIBRARIES(${EXECUTABLE}  net_rpc)
#ADD_TEST(${EXECUTABLE}              ${EXECUTABLE})
