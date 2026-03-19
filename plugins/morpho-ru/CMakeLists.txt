project(morpho_ru)

find_package(MoonyCode REQUIRED)
find_package(MorphRus REQUIRED)

add_compile_options(-static-libstdc++ -fvisibility=hidden -fPIC)

add_library(morpho-ru SHARED morpho-ru.cpp)

if (NOT TARGET mtc)
  add_subdirectory(contrib/mtc)
endif()

add_executable(
  test-palmira-plugins-morpho-ru tests/test-morpho-ru.cpp)
target_link_libraries(
  test-palmira-plugins-morpho-ru morpho-ru)

target_link_libraries(morpho-ru ${MorphRus_LIBRARY} ${MoonyCode_LIBRARY} mtc)
