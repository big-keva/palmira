cmake_minimum_required(VERSION 3.5)
set(PROJECT_NAME palmira)
project(${PROJECT_NAME})

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

option(SANITIZE_ENABLED "Enable sanitize" OFF)
option(PROFILER_ENABLED "Enable profile" OFF)

option(USE_BUILTIN_REST "Enable builtin RestAPI library" ON)

option(BUILD_MTC_ENABLED "Enable build mtc module" OFF)
option(BUILD_MOONYCODE_ENABLED "Enable build moonycode module" OFF)

set(REGISTER_MODULES {};)
set(MODULE_LIBS)

# Set output folders
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib CACHE PATH "Single Directory for all")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib CACHE PATH "Single Directory for all")

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

if(USE_BUILTIN_REST)
  find_package(simdjson REQUIRED)

  set(MODULE_LIBS ${MODULE_LIBS} $<LINK_LIBRARY:WHOLE_ARCHIVE,palmira-restAPI> uSockets ssl crypto simdjson::simdjson)
endif()

include_directories(
  ./
  contrib)

if (BUILD_MTC_ENABLED)
	add_subdirectory(contrib/mtc)
endif (BUILD_MTC_ENABLED)

set(MoonyCode_LIB moonycode-objset)
set(MoonyCode_LIB moonycode-objset)
if (BUILD_MOONYCODE_ENABLED)
	add_subdirectory(contrib/moonycode)
	message(STATUS "Found ${MoonyCode_LIB}" )

	add_subdirectory(contrib/libmorph)
	set(MorphRus_LIB  morphrus-objset)
	set(FuzzyRus_LIB  fuzzyrus-objset)
else ()
	find_package(MoonyCode REQUIRED)
endif (BUILD_MOONYCODE_ENABLED)

add_subdirectory(contrib/DeliriX)
add_subdirectory(contrib/structo)
add_subdirectory(contrib/hopcalc)

add_subdirectory(plugins/morpho-ru)

set_target_properties(DeliriX PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties(mtc PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(NETWORK_SOURCE_DIR ${PROJECT_SOURCE_DIR}/network)

add_subdirectory(network/restAPI)
add_subdirectory(network/grpcAPI)

add_library(tripoli
  src/objects/read-zip.cpp
  src/objects/doc-zmap.cpp

  src/service/structo-create.cpp
  src/service/structo-search.cpp
  src/service/collect-quotes.cpp
  src/service/quotation-tool.cpp
  src/service/collect-docs.cpp
  src/service/collect-dump.cpp
  src/toolset/zip-unzip.cpp

  src/toolset/plugins.cpp
  src/toolset/toolset.cpp
  src/toolset/modules.cpp
  src/toolset/service.cpp)
set_target_properties(tripoli PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(watchFs
	src/watchFs/watchFs${XXX_SUF}.cpp)

add_executable(${PROJECT_NAME}
	src/palmira.cpp
	src/servers.cpp)

target_link_libraries(${PROJECT_NAME}
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

#[[
add_executable(k2
    src/k2.cpp)

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
]]

# add_subdirectory(utils)
# add_subdirectory(tests)
