cmake_minimum_required (VERSION 3.0)

project(Progression)

option(BUILD_EXAMPLES "Build the example programs" OFF)

if (MSVC)
    set(CPP_VERSION "/std:c++17")
else()
    set(CPP_VERSION "-std=c++17")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CPP_VERSION} -g")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_RELEASE} ${CPP_VERSION} -O3")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (NOT PROGRESSION_DIR)
    set(PROGRESSION_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

if (WIN32)
    message(STATUS "Starting build for Windows system...")

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/lib)


elseif(LINUX)
    message(STATUS "Starting build for linux systems...")
endif()


add_subdirectory(project/)


if (BUILD_EXAMPLES)
    add_subdirectory(examples/)
endif()
