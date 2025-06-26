add_library(palmira-tools
	tools/primes.cxx)

add_library(palmira-index
	storage/posix-fs-policies.cpp
	storage/posix-fs-output.cpp
	storage/posix-fs-serial.cpp
	storage/posix-fs-storage.cpp

        index/contents-index.cxx
	index/override-entities.cxx

	index/dynamic-contents.cxx
	index/static-contents.cxx
	index/commit-contents.cxx
	index/merger-contents.cxx

        index/index-layers.cxx

        index/contents-index-fusion.cxx
                
	index/layered-contents.cxx)

# add_library(palmira-letters
#         letters/letters.cpp)

add_subdirectory(tests)
add_subdirectory(utils)
	