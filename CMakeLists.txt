cmake_minimum_required (VERSION 3.8)

project(Progression LANGUAGES CXX C)

option(PROGRESSION_BUILD_EXAMPLES "Build the example programs" ON)
option(PROGRESSION_BUILD_TOOLS "Build the tools, such as the OBJ converter" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LINUX_PROGRAM   "NOT_IN_USE")
set(WINDOWS_PROGRAM "NOT_IN_USE")
set(APPLE_PROGRAM   "NOT_IN_USE")

if (UNIX AND NOT APPLE)
    set(LINUX_PROGRAM   "IN_USE")
elseif (WIN32)
    set(WINDOWS_PROGRAM "IN_USE")
elseif (APPLE)
    set(APPLE_PROGRAM   "IN_USE")
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Ship")

if (UNIX AND NOT APPLE)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Werror -Wextra -Wno-implicit-fallthrough -Wshadow -Wno-unused-variable -Wno-unused-function")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CPP_VERSION} -g")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CPP_VERSION} -O2")
    set(CMAKE_CXX_FLAGS_SHIP "${CMAKE_CXX_FLAGS_SHIP} ${CPP_VERSION} -O3")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /WX")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ox")
    set(CMAKE_CXX_FLAGS_SHIP "${CMAKE_CXX_FLAGS_SHIP} /O2")
endif()

if (MSVC)
    SET(CMAKE_CXX_FLAGS_SHIP
        "${CMAKE_CXX_FLAGS_SHIP} /O2"
        CACHE STRING "Flags used by the C++ compiler during coverage builds."
        FORCE )
    SET(CMAKE_C_FLAGS_SHIP
        ""
        CACHE STRING "Flags used by the C compiler during coverage builds."
        FORCE )
    SET(CMAKE_EXE_LINKER_FLAGS_SHIP
        ""
        CACHE STRING "Flags used for linking binaries during coverage builds."
        FORCE
    )
    SET(CMAKE_SHARED_LINKER_FLAGS_SHIP
        ""
        CACHE STRING "Flags used by the shared libraries linker during coverage builds."
        FORCE
    )
    MARK_AS_ADVANCED(
        CMAKE_CXX_FLAGS_SHIP
        CMAKE_C_FLAGS_SHIP
        CMAKE_EXE_LINKER_FLAGS_SHIP
        CMAKE_SHARED_LINKER_FLAGS_SHIP
    )
else()

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
add_subdirectory(ext/lz4)

add_subdirectory(progression)

if (PROGRESSION_BUILD_EXAMPLES)
    add_subdirectory(examples/)
endif()

if (PROGRESSION_BUILD_TOOLS)
    add_subdirectory(tools/)
endif()
