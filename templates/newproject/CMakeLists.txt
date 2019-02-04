cmake_minimum_required (VERSION 3.8)

project(##projectName##)

set(PROGRESSION_DIR ##PROGRESSION_DIR##)
set(PROGRESSION_BUILD_EXAMPLES OFF CACHE BOOL " " FORCE)
add_subdirectory(${PROGRESSION_DIR} ${PROGRESSION_DIR}/build)
list(APPEND CMAKE_MODULE_PATH "${PROGRESSION_DIR}/cmake")
include(Progression)

configure_file(${CMAKE_SOURCE_DIR}/cmake/configuration.hpp.in ${CMAKE_SOURCE_DIR}/code/configuration.hpp)

set(
    SRCS
    code/main.cpp
    code/##projectName##.cpp
)

set(
    HEADERS
    code/configuration.hpp
    code/##projectName##.hpp
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
add_executable(##projectName## ${SRCS} ${HEADERS})

if(MSVC)
    target_compile_options(##projectName## PRIVATE "/MP")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ##projectName##)
endif()

set_target_properties(
    ##projectName##
    PROPERTIES
    DEBUG_POSTFIX _debug
)

target_link_libraries(##projectName## ${PROGRESSION_LIBS})
include_directories(code)
include_directories(${PROGRESSION_INCLUDE_DIRS})
