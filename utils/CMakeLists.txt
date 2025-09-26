link_libraries(
	palmira
	DelphiX
	moonycode
	mtc
	tinyxml2
	minizip
	z
	pthread
	dl)

add_executable(libruseq-local
	libruseq/local-indexer.cpp)

add_executable(k2-local
	k2-local/k2.cpp
	k2-local/watch-fs.cpp)

target_link_libraries(k2-local
	remottp
	moonycode
	mtc)
