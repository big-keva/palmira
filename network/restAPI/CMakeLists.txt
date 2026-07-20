project(restAPI)

set(TARGET
  palmira-restAPI)

set(SOURCE
  src/delete-entity.cpp
  src/update-entity.cpp
  src/insert-entity.cpp
  src/responder.cpp
  src/client.cpp
  src/jsload.cpp
  src/server.cpp
  src/unpack.cpp
  src/zmload.cpp)

if(MSVC)
  set(EXPORT_TABLE ${NETWORK_SOURCE_DIR}/network.msvc.def)
else()
  set(SYMBOLS_FILE ${NETWORK_SOURCE_DIR}/network.unix.def)

  add_compile_options(-W -Wall -Wextra)
  add_compile_options(-fPIC)

  if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-fno-stack-protector -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0)
  else()
    add_compile_options(-fstack-protector-all -g)
  endif()

  set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--dynamic-list=${SYMBOLS_FILE} -static-libgcc -static-libstdc++" )
endif()

if(USE_BUILTIN_REST)
  add_library(${TARGET} STATIC ${SOURCE} src/static.cpp)
else()
  add_library(${TARGET} SHARED ${SOURCE} src/shared.cpp)

  target_link_libraries(${TARGET}
    structo
    DeliriX
    moonycode
    mtc)
endif()
