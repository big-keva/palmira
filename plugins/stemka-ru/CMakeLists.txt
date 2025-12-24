project(stemka_ru)

add_compile_options(-static-libstdc++ -fvisibility=hidden -fPIC)

if (NOT TARGET stemka)
	add_subdirectory(contrib/stemka)
endif()

add_library(stemka-ru SHARED
	src/stemming-rus.cpp
	src/language-api.cpp)

target_link_libraries(stemka-ru stemka moonycode)

add_subdirectory(tests)
