cmake_minimum_required(VERSION 3.1)
project(palmira)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

include_directories(
	./
	contrib)

add_compile_options(-mfpmath=sse -fstack-protector-all -g
	-W -Wall -Wextra -Wcast-align -pedantic -Wfloat-equal -Wpointer-arith -Wformat-security
	-Wmissing-format-attribute -Wformat=1 -Wwrite-strings -Wcast-align -Wno-long-long
	-Woverloaded-virtual -Wno-suggest-attribute=format)

add_subdirectory(contrib/moonycode)
add_subdirectory(contrib/mtc)
add_subdirectory(contrib/DelphiX)

add_library(palmira
	src/tinyxml/load-xml-text.cpp

	service/delphi-search.cpp
	service/z-arguments.cpp)

add_subdirectory(tests)
