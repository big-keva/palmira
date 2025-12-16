cmake_minimum_required(VERSION 3.20)

project(remoapi)

add_library(remoapi
	src/jsload.cpp
	src/zmload.cpp
	src/unpack.cpp
	src/server.cpp
	src/client.cpp)
