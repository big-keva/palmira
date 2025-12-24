project(stemka_ua)

add_compile_options(-static-libstdc++ -fvisibility=hidden -fPIC)

if (NOT TARGET stemka)
	add_subdirectory(contrib/stemka)
endif()

add_library(stemka-ua SHARED
	src/stemming-ukr.cpp
	src/language-api.cpp)

target_link_libraries(stemka-ua stemka moonycode)

add_subdirectory(tests)
