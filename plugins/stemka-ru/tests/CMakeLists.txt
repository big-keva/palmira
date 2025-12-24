add_compile_definitions(
	STEMKA_RU_SO_PATH=${stemka_ru_BINARY_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}stemka-ru${CMAKE_SHARED_LIBRARY_SUFFIX})

add_executable(test-stemka-ru
	test-stemka-ru.cpp
	test-main.cpp)

add_dependencies(test-stemka-ru stemka-ru)

target_link_libraries(test-stemka-ru stemka structo moonycode mtc)
