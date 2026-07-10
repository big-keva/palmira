# Checking dependency modules.
find_package(pybind11 QUIET)

if (NOT pybind11_FOUND)
    message(FATAL_ERROR "The module pybind11 not found. Please, install it by sudo apt-get install pybind11-dev")
else ()
    message(STATUS "Found pybind11 version: ${pybind11_VERSION}")
endif ()
