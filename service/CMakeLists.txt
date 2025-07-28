add_compile_options(-fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=leak)
add_link_options(-fsanitize=address)

add_executable(delphi-search
      	delphi-search.cpp
	z-arguments.cpp)

target_link_libraries(delphi-search
	DelphiX
	moonycode
	mtc
	pthread
	dl)
