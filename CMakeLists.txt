cmake_minimum_required (VERSION 3.8)

project(Progression)

option(PROGRESSION_BUILD_EXAMPLES "Build the example programs" ON)

set(CMAKE_CXX_STANDARD 17)
if (MSVC)
    add_compile_options("/std:c++17")
endif()


#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CPP_VERSION} -g")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_RELEASE} ${CPP_VERSION} -O3")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (NOT PROGRESSION_DIR)
    set(PROGRESSION_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/lib)

# for the config manager
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "disable yaml-cpp tests build")
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "disable yaml-cpp tools build")
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "disable yaml-cpp contrib build")
add_subdirectory(ext/yaml-cpp EXCLUDE_FROM_ALL)

# Disable building extras we won't need (pure C++ project)
set(NANOGUI_BUILD_EXAMPLE OFF CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_PYTHON  OFF CACHE BOOL " " FORCE)
set(NANOGUI_INSTALL       OFF CACHE BOOL " " FORCE)

# Add the configurations from nanogui
add_subdirectory(ext/nanogui)

include_directories(${NANOGUI_EXTRA_INCS})

# For reliability of parallel build, make the NanoGUI targets dependencies

add_subdirectory(progression/)

#set_property(progression glfw nanogui PROPERTY FOLDER "dependencies")

if (PROGRESSION_BUILD_EXAMPLES)
    add_subdirectory(examples/)
endif()
