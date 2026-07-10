include(FetchContent)

# Checking the internet on available.
if (NOT CMAKE_DISABLE_FETCH_CONTENT)
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v2.10.4
        GIT_SHALLOW TRUE
    )

    FetchContent_MakeAvailable(pybind11)

    message(STATUS "pybind11 downloaded and configured successfully")
else ()
    message(FATAL_ERROR "
    ================================================================
    pybind11 not found and automatic download is disabled!

    Please install pybind11-dev manually or enable FetchContent.
    ================================================================
    ")
endif ()