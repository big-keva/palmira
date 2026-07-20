cmake_minimum_required(VERSION 3.5)
project(palmira)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

option(SANITIZE_ENABLED "Enable sanitize" OFF)
option(PROFILER_ENABLED "Enable profile" OFF)

option(USE_BUILTIN_REST "Enable builtin RestAPI library" ON)

set(REGISTER_MODULES {};)

if(MSVC)
  add_compile_options(-D_CRT_SECURE_NO_WARNINGS)
  set(XXX_SUF -win)
else()
  add_compile_options(-mfpmath=sse -fstack-protector-all -g
    -W -Wall -Wextra -Wcast-align -pedantic -Wfloat-equal -Wpointer-arith -Wformat-security
    -Wmissing-format-attribute -Wformat=1 -Wwrite-strings -Wcast-align -Wno-long-long
    -Woverloaded-virtual -Wno-suggest-attribute=format)
  add_compile_options(-fPIC)
  set(XXX_SUF -nix)

  if (SANITIZE_ENABLED)
    add_compile_options(-fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=leak)
    add_link_options(-fsanitize=address)
  endif()

  if (PROFILER_ENABLED)
    add_compile_options(-p -pg)
    add_link_options(-p -pg)
  endif()
endif()

find_package(MoonyCode REQUIRED)

include_directories(
	./
	contrib)

add_subdirectory(contrib/mtc)
add_subdirectory(contrib/DeliriX)
add_subdirectory(contrib/structo)
add_subdirectory(contrib/hopcalc)

add_subdirectory(plugins/morpho-ru)

set(NETWORK_SOURCE_DIR ${PROJECT_SOURCE_DIR}/network)

add_subdirectory(network/restAPI)
add_subdirectory(network/grpcAPI)

add_library(tripoli
  src/objects/read-zip.cpp
  src/objects/doc-zmap.cpp

  src/service/structo-create.cpp
  src/service/structo-search.cpp
  src/service/collect-quotes.cpp
  src/service/collect-docs.cpp

  src/toolset/plugins.cpp
  src/toolset/toolset.cpp
  src/toolset/modules.cpp
  src/toolset/z-arguments.cpp)

add_library(watchFs
	src/watchFs/watchFs${XXX_SUF}.cpp)

add_executable(palmira
	src/palmira.cpp
	src/servers.cpp)

add_executable(k2
	src/k2.cpp)

if(USE_BUILTIN_REST)
  set(MODULE_LIBS ${MODULE_LIBS} $<LINK_LIBRARY:WHOLE_ARCHIVE,palmira-restAPI> uSockets ssl crypto)
endif()

target_link_libraries(palmira
	${MODULE_LIBS}
	tripoli
	structo
	DeliriX
	${MoonyCode_LIB}
	mtc
	tinyxml2
	minizip
	z
	pthread
	dl)

target_link_libraries(k2
	${MODULE_LIBS}
	tripoli
	structo
	DeliriX
	watchFs
	${MoonyCode_LIB}
	mtc
	tinyxml2
	minizip
	z
	pthread
	dl)

add_subdirectory(utils)
add_subdirectory(tests)
