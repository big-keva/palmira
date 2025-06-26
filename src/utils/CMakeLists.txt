add_executable(merge-indices
	merge-indices.cxx)

target_link_libraries(merge-indices
	palmira-index
	palmira-tools
	mtc)
