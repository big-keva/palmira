add_library(palmira-tools
	tools/primes.cxx)

add_library(palmira-index
	storage/filesystem-policies.cxx
	storage/filesystem-output.cxx
	storage/filesystem-serial.cxx
	storage/filesystem-storage.cxx

        index/contents-index.cxx
	index/override-entities.cxx

	index/dynamic-contents.cxx
	index/static-contents.cxx
	index/commit-contents.cxx

        index/index-layers.cxx

	index/layered-contents.cxx)

add_subdirectory(tests)
	