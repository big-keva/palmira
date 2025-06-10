add_library(palmira-tools
	tools/primes.cxx)

add_library(palmira-index
	storage/filesystem-policies.cxx
	storage/filesystem-output.cxx
	storage/filesystem-serial.cxx

        index/contents-index.cxx
	index/dynamic-contents-index.cxx
	index/static-contents-index.cxx
	index/commit-contents-index.cxx
	index/flaked-contents-index.cxx)

add_subdirectory(tests)
	