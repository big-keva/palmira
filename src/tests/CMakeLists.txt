link_libraries(palmira-tools mtc)

add_executable(test-palmira-index
	index/test-dynamic-entities.cxx
	index/test-dynamic-chains.cxx
	test-main.cxx)
