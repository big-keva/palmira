cmake_minimum_required(VERSION 3.1)
project(palmira)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

option(SANITIZE_ENABLED "Enable sanitize" ON)
option(PROFILER_ENABLED "Enable profile" ON)

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

add_subdirectory(contrib/moonycode)
add_subdirectory(contrib/mtc)
add_subdirectory(contrib/DeliriX)
add_subdirectory(contrib/structo)
add_subdirectory(contrib/remottp)

add_library(palmira
	src/objects/read-zip.cpp
	src/objects/doc-zmap.cpp

	src/service/delphi-search.cpp
	src/service/collect-docs.cpp
	src/service/collect-quotes.cpp

	src/z-arguments.cpp)

add_subdirectory(tests)
add_subdirectory(utils)
