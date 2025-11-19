link_libraries(
	palmira
	DelphiX
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

add_subdirectory(k2-local)
