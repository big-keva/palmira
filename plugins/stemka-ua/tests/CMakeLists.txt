add_compile_definitions(
	STEMKA_UA_SO_PATH=${stemka_ua_BINARY_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}stemka-ua${CMAKE_SHARED_LIBRARY_SUFFIX})

add_executable(test-stemka-ua
	test-stemka-ua.cpp
	test-main.cpp)

add_dependencies(test-stemka-ua stemka-ua)

target_link_libraries(test-stemka-ua stemka structo moonycode mtc)
