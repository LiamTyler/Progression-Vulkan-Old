cmake_minimum_required (VERSION 3.8)

project(Progression LANGUAGES CXX C)

option(PROGRESSION_BUILD_EXAMPLES "Build the example programs" ON)
option(PROGRESSION_BUILD_TOOLS "Build the tools, such as the OBJ converter" ON)

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

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_SHIP ${CMAKE_BINARY_DIR}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_SHIP ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_SHIP ${CMAKE_BINARY_DIR}/lib )

if (UNIX AND NOT APPLE)
    #set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Werror -Wextra -Wno-implicit-fallthrough -Wshadow -Wno-unused-variable -Wno-unused-function")
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wno-unused-function")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
    set(CMAKE_CXX_FLAGS_SHIP "${CMAKE_CXX_FLAGS_SHIP} -O3") # look up Ofast
elseif(MSVC)
    #set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /WX")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
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

set(GLFW_BUILD_EXAMPLES  OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_TESTS     OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_DOCS      OFF CACHE BOOL " " FORCE)

add_subdirectory(ext/glfw)

set(ASSIMP_BUILD_TESTS      OFF CACHE BOOL " " FORCE)
add_subdirectory(ext/assimp)


if (UNIX AND NOT APPLE)
    add_definitions( -DLUA_USE_POSIX )
endif()
file(GLOB_RECURSE LUA_SRC ext/lua/*.c)
list(REMOVE_ITEM LUA_SRC "${CMAKE_SOURCE_DIR}/ext/lua/lua.c")
list(REMOVE_ITEM LUA_SRC "${CMAKE_SOURCE_DIR}/ext/lua/luac.c")
add_library(lua ${LUA_SRC})
set_target_properties(
    lua
    PROPERTIES
    DEBUG_POSTFIX _debug
    RELEASE_POSTFIX _release
    SHIP_POSTFIX _ship
)

add_subdirectory(ext/basis_universal)
add_subdirectory(ext/meshoptimizer)
add_subdirectory(ext/lz4)

add_subdirectory(progression)

if (PROGRESSION_BUILD_EXAMPLES)
    add_subdirectory(examples/)
endif()

if (PROGRESSION_BUILD_TOOLS)
    add_subdirectory(tools/)
endif()
