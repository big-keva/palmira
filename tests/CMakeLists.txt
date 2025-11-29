add_compile_options(-fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=leak)
add_link_options(-fsanitize=address)

link_libraries(palmira structo tinyxml2 mtc moonycode minizip z)

add_executable(test-palmira-zipfile
	readers/test-zipfile.cpp
	test-main.cpp)

add_executable(test-palmira-service
	test-main.cpp)
