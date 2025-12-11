cmake_minimum_required(VERSION 3.20)

project(remottp-api)

add_library(remottp-api
	src/jsload.cpp
	src/zmload.cpp
	src/unpack.cpp
	src/server.cpp)
