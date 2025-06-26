add_library(palmira-tools
	tools/primes.cxx)

add_library(palmira-index
	storage/posix-fs-policies.cpp
	storage/posix-fs-output.cpp
	storage/posix-fs-serial.cpp
	storage/posix-fs-storage.cpp

        indexes/contents-index.cxx
	indexes/override-entities.cxx

	indexes/dynamic-contents.cxx
	indexes/static-contents.cxx
	indexes/commit-contents.cxx
	indexes/merger-contents.cxx

        indexes/index-layers.cxx

        indexes/contents-index-fusion.cxx
                
	indexes/layered-contents.cxx)

# add_library(palmira-letters
#         letters/letters.cpp)

add_subdirectory(utils)
	