include_directories(include)

# Building common library of modules.
add_subdirectory(common)

# Building modules.
add_subdirectory(docapi-http)
