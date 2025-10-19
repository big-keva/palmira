set(COMMON tmppath.cpp test-main.cpp)

add_executable(test-k2-watch-fs
	test-watch-fs.cpp
	../watch-fs${XXX_SUF}.cpp ${COMMON})

target_link_libraries(test-k2-watch-fs
	moonycode
	mtc)
