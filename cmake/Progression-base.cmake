if(NOT GLEW_DIR)
    set(GLEW_DIR C:/GLEW)
endif()

if (NOT SDL2_DIR)
    set(SDL2_DIR C:/SDL2)
endif()

if (NOT PROGRESSION_DIR)
    set(PROGRESSION_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# Find OpenGL
find_package(OpenGL REQUIRED)

# Find GLEW
find_library(GLEW_LIBRARY
    NAMES glew32s
    HINTS ${GLEW_DIR}/lib/Release/x64
    )
if (NOT GLEW_LIBRARY)
    message(FATAL_ERROR "Could not find the GLEW Library. Exiting")
endif()

set(GLEW_INCLUDE_DIR ${GLEW_DIR}/include)

# Find SDL2
find_library(SDL2_LIBRARY
    NAMES SDL2
    HINTS ${SDL2_DIR}/lib/x64
    )
if (NOT SDL2_LIBRARY)
    message(FATAL_ERROR "Could not find the SDL2 Library. Exiting")
endif()

find_library(SDL2_LIBRARY_MAIN
    NAMES SDL2main
    HINTS ${SDL2_DIR}/lib/x64
    )
if (NOT SDL2_LIBRARY_MAIN)
    message(FATAL_ERROR "Could not find the SDL2main Library. Exiting")
endif()

set(SDL2_INCLUDE_DIR ${SDL2_DIR}/include)

set(PROGRESSION_INCLUDE_DIRS
    ${GLEW_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIR}
    ${PROGRESSION_DIR}/ext/
    ${PROGRESSION_DIR}/include/
    ${PROGRESSION_DIR}/project/
    ${PROGRESSION_DIR}/project/src
    )

set(PROGRESSION_LIBS
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARY}
    ${SDL2_LIBRARY}
    ${SDL2_LIBRARY_MAIN}
    )

include_directories(${PROGRESSION_INCLUDE_DIRS})
