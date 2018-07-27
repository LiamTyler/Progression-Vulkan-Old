cmake_minimum_required (VERSION 3.8)

project(Progression)

option(BUILD_EXAMPLES "Build the example programs" OFF)

if (MSVC)
    set(CPP_VERSION "/std:c++17")
else()
    set(CPP_VERSION "-std=c++17")
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

# Compile GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL " " FORCE)
set(GLFW_BUILD_INSTALL OFF CACHE BOOL " " FORCE)
set(GLFW_INSTALL OFF CACHE BOOL " " FORCE)
set(GLFW_USE_CHDIR OFF CACHE BOOL " " FORCE)
add_subdirectory(ext/glfw)

set(GLFW_LIB glfw)
set(GLFW_INCLUDES ${CMAKE_SOURCE_DIR}/ext/glfw/include)

set(GLAD_ROOT_DIR ${CMAKE_SOURCE_DIR}/ext/glad)
set(GLAD_INCLUDE_DIR ${GLAD_ROOT_DIR}/include)
set(GLAD_SOURCES ${GLAD_ROOT_DIR}/src/glad.c)
set(GLAD_INCLUDES
    ${GLAD_INCLUDE_DIR}/glad/glad.h
    ${GLAD_INCLUDE_DIR}/KHR/khrplatform.h
)

# Disable building extras we won't need (pure C++ project)
set(NANOGUI_BUILD_EXAMPLE OFF CACHE BOOL " " FORCE)
set(NANOGUI_BUILD_PYTHON  OFF CACHE BOOL " " FORCE)
set(NANOGUI_INSTALL       OFF CACHE BOOL " " FORCE)

# Add the configurations from nanogui
add_subdirectory(ext/nanogui)

include_directories(${NANOGUI_EXTRA_INCS})

# For reliability of parallel build, make the NanoGUI targets dependencies

add_subdirectory(project/)

#set_property(progression glfw nanogui PROPERTY FOLDER "dependencies")

if (PROGRESSION_BUILD_EXAMPLES)
    add_subdirectory(examples/)
endif()
