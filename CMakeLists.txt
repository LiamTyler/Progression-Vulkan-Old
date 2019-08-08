cmake_minimum_required (VERSION 3.8)

project(Progression)

option(PROGRESSION_BUILD_EXAMPLES "Build the example programs" ON)
option(PROGRESSION_BUILD_TOOLS "Build the tools, such as the OBJ converter" ON)
option(PROGRESSION_AUDIO "Enable the progression audio system" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (UNIX AND NOT APPLE)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Werror -Wextra -Wno-implicit-fallthrough -Wshadow -Wno-unused-variable -Wno-unused-function")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CPP_VERSION} -g")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CPP_VERSION} -O3")
    #option(PROGRESSION_AUDIO "Enable the progression audio system" ON)
elseif (MSVC)
    add_compile_options("/std:c++14")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (NOT PROGRESSION_DIR)
    set(PROGRESSION_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

# Disable building extras we won't need (pure C++ project)
set(GLFW_BUILD_EXAMPLES  OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_TESTS     OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_DOCS      OFF CACHE BOOL " " FORCE)

add_subdirectory(ext/glfw)
add_subdirectory(ext/meshoptimizer)

add_subdirectory(progression)

if (PROGRESSION_BUILD_EXAMPLES)
    add_subdirectory(examples/)
endif()

if (PROGRESSION_BUILD_TOOLS)
    add_subdirectory(tools/)
endif()
