add_executable(test-palmira-restAPI-client
  client/test-client.cpp
  ../../../src/toolset/service.cpp
  ../../../src/objects/doc-zmap.cpp)

target_link_libraries(test-palmira-restAPI-client
  palmira-restAPI
  ixwebsocket
  uSockets ssl crypto
  DeliriX
  mtc
  moonycode)