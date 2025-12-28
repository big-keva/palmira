link_libraries(
	tripoli
	structo
	DeliriX
	moonycode
	mtc
	tinyxml2
	minizip
	z
	pthread
	dl)

add_executable(libruseq-index
	libruseq/local-indexer.cpp)

add_executable(libruseq-sender
	libruseq/sender.cpp)

add_executable(libruseq-send-1
	libruseq/send-1.cpp)
