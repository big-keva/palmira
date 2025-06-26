link_libraries(palmira-index palmira-tools mtc moonycode)

add_executable(test-palmira-index
        storage/test-storage-fs-based.cxx

	index/test-dynamic-chains-ringbuffer.cxx
	index/test-dynamic-entities.cxx
	index/test-dynamic-chains.cxx
	index/test-dynamic-contents.cxx

	index/test-static-entities.cxx
	index/test-static-contents.cxx

	index/test-patch-table.cxx

	index/test-commit-contents.cxx

        index/test-index-layers.cxx

        index/test-layered-contents.cxx

        index/test-stream-indexing.cxx

	test-main.cxx)

add_executable(test-palmira-letters
	letters/test-letters.cpp)
