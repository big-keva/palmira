link_libraries(palmira-index palmira-texts palmira-context palmira-tools mtc moonycode)

add_executable(test-palmira-index
        storage/test-storage-fs-based.cpp

	indexes/test-dynamic-chains-ringbuffer.cpp
	indexes/test-dynamic-entities.cpp
	indexes/test-dynamic-chains.cpp
	indexes/test-dynamic-contents.cpp

	indexes/test-static-entities.cpp
	indexes/test-static-contents.cpp

	indexes/test-patch-table.cpp
	indexes/test-commit-contents.cpp
        indexes/test-index-layers.cpp
        indexes/test-layered-contents.cpp
        indexes/test-stream-indexing.cpp

	texts/test-letters.cpp

	test-main.cpp)

add_executable(test-palmira-texts
	texts/test-letters.cpp

	test-main.cpp)

add_executable(test-palmira-context
	context/test-key.cpp
	context/test-contents-processor.cpp

	test-main.cpp)

add_executable(test-palmira-hunspell
	texts/test-hunspell.cpp)

target_link_libraries(test-palmira-hunspell
	hunspell)