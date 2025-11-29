link_libraries(
	palmira
	structo
	DeliriX
	remottp
	moonycode
	mtc
	tinyxml2
	minizip
	z
	pthread
	dl)

add_executable(libruseq-sender
	libruseq/sender.cpp)

add_executable(libruseq-search
	libruseq/search.cpp)

add_executable(libruseq-send-1
	libruseq/send-1.cpp)

add_subdirectory(k2-local)
