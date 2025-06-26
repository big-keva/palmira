link_libraries(palmira-index palmira-tools mtc moonycode)

add_executable(test-palmira-index
        storage/test-storage-fs-based.cxx

	indexes/test-dynamic-chains-ringbuffer.cxx
	indexes/test-dynamic-entities.cxx
	indexes/test-dynamic-chains.cxx
	indexes/test-dynamic-contents.cxx

	indexes/test-static-entities.cxx
	indexes/test-static-contents.cxx

	indexes/test-patch-table.cxx

	indexes/test-commit-contents.cxx

        indexes/test-index-layers.cxx

        indexes/test-layered-contents.cxx

        indexes/test-stream-indexing.cxx

	test-main.cxx)

add_executable(test-palmira-letters
	letters/test-letters.cpp)
