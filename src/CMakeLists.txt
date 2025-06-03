add_library(palmira-tools
	tools/primes.cxx)

add_library(palmira-index
	storage/filesystem-policies.cxx
	storage/filesystem-output.cxx
	storage/filesystem-serial.cxx

	index/dynamic-contents.cxx)

add_subdirectory(tests)
	