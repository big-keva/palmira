add_compile_options(-fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=leak)
add_link_options(-fsanitize=address)

link_libraries(palmira DelphiX tinyxml2 mtc moonycode)

add_executable(test-palmira-tinyxml
	tinyxml/test-tinyxml.cpp
	test-main.cpp)

add_executable(test-palmira-service
	test-main.cpp)
