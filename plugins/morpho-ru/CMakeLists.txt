project(morpho_ru)

add_compile_options(-static-libstdc++ -fvisibility=hidden -fPIC)

add_library(morpho-ru SHARED
	morpho-ru.cpp)

if (NOT TARGET moonycode)
  add_subdirectory(contrib/moonycode)
endif()

if (NOT TARGET mtc)
  add_subdirectory(contrib/mtc)
endif()

if (NOT TARGET morphrus)
  add_subdirectory(contrib/libmorph)
endif()

target_link_libraries(morpho-ru morphrus fuzzyrus moonycode mtc)
