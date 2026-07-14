cmake_minimum_required(VERSION 3.5)
project(palmira)

set(I_AM_KEVA ON)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

if(MSVC)
	set(XXX_SUF -win)
else()
	set(XXX_SUF -nix)
endif()

option(SANITIZE_ENABLED "Enable sanitize" OFF)
option(PROFILER_ENABLED "Enable profile" OFF)

# add_compile_options(-DDEBUG_TOOLS)
#
add_compile_options(-mfpmath=sse -fstack-protector-all -g
	-W -Wall -Wextra -Wcast-align -pedantic -Wfloat-equal -Wpointer-arith -Wformat-security
	-Wmissing-format-attribute -Wformat=1 -Wwrite-strings -Wcast-align -Wno-long-long
	-Woverloaded-virtual -Wno-suggest-attribute=format)

if (SANITIZE_ENABLED)
	add_compile_options(-fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=leak)
	add_link_options(-fsanitize=address)
endif()

if (PROFILER_ENABLED)
	add_compile_options(-p -pg)
	add_link_options(-p -pg)
endif()

include_directories(
	./
	contrib)

add_subdirectory(contrib/mtc)

if(I_AM_KEVA)
  add_subdirectory(contrib/moonycode)
    set(MoonyCode_LIB moonycode-objset)

  message(STATUS "Found ${MoonyCode_LIB}" )

  add_subdirectory(contrib/libmorph)
    set(MorphRus_LIB  morphrus-objset)
    set(FuzzyRus_LIB  fuzzyrus-objset)
endif()

if("${MoonyCode_LIB}" STREQUAL "")
  find_package(MoonyCode REQUIRED)
endif()

add_subdirectory(contrib/DeliriX)
add_subdirectory(contrib/structo)
add_subdirectory(contrib/hopcalc)

add_subdirectory(plugins/morpho-ru)

set(NETWORK_SOURCE_DIR ${PROJECT_SOURCE_DIR}/network)

add_subdirectory(network/grpcAPI)
add_subdirectory(network/restAPI)

add_library(tripoli
	src/objects/read-zip.cpp
	src/objects/doc-zmap.cpp

	src/service/structo-create.cpp
	src/service/structo-search.cpp
	src/service/collect-quotes.cpp
	src/service/collect-docs.cpp

	src/toolset/plugins.cpp
	src/toolset/toolset.cpp
	src/toolset/z-arguments.cpp)

add_library(watchFs
	src/watchFs/watchFs${XXX_SUF}.cpp)

add_executable(palmira
	src/palmira.cpp
	src/servers.cpp)

add_executable(k2
	src/k2.cpp)

target_link_libraries(palmira
	${GRPC_LIBS}
#	httpapi
	tripoli
	structo
	DeliriX
#	remottp
	${MoonyCode_LIB}
	mtc
	tinyxml2
	minizip
	z
	pthread
	dl)

target_link_libraries(k2
	remottp
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
