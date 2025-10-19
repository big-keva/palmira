if(MSVC)
	set(XXX_SUF -win)
else()
	set(XXX_SUF -nix)
endif()

add_executable(k2-local
	k2.cpp
	watch-fs${XXX_SUF}.cpp)

target_link_libraries(k2-local
	remottp
	palmira
	DelphiX
	moonycode
	mtc
	tinyxml2
	minizip
	z
	pthread
	dl)

add_subdirectory(tests)
