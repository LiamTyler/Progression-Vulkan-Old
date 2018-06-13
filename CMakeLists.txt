cmake_minimum_required (VERSION 3.0)

project(OpenGL_Starter)

option(BUILD_EXAMPLES "Build the example programs" ON)

if (WIN32)
    message(STATUS "Starting build for Windows system...")

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/lib)

    # Path to main directory of GLEW and SDL2
    set(GLEW_DIR C:/GLEW)
    set(SDL_DIR C:/SDL2)

    # store the location of the DLL's
    set(GLEW_DLL ${GLEW_DIR}/bin/Release/x64/glew32.dll)
    set(SDL_DLL ${SDL_DIR}/lib/x64/SDL2.dll)

    file(COPY ${GLEW_DLL} DESTINATION ${CMAKE_BINARY_DIR}/bin)
    file(COPY ${SDL_DLL}  DESTINATION ${CMAKE_BINARY_DIR}/bin)

    set(INCLUDE_DIRS
        ${GLEW_DIR}/include
        ${SDL_DIR}/include
        ${CMAKE_SOURCE_DIR}/ext/
        ${CMAKE_BINARY_DIR}/
        ${CMAKE_SOURCE_DIR}/project/
        ${CMAKE_SOURCE_DIR}/project/src
        )

    set(LIB_DIRS
        ${GLEW_DIR}/lib/Release/x64
        ${SDL_DIR}/lib/x64
        ${CMAKE_BINARY_DIR}/lib
        )

    set(EXTERNAL_LIBS
        opengl32.lib
        glew32s.lib
        SDL2.lib
        SDL2main.lib
        )

    set(OGL_STARTER_LIB OpenGL_Starter)
    set(OGL_STARTER_DEBUG_LIB OpenGL_Starter_debug)

    include_directories(${INCLUDE_DIRS})
    link_directories(${LIB_DIRS})


elseif(LINUX)
    message(STATUS "Starting build for linux systems...")
endif()

add_subdirectory(project/)
if (BUILD_EXAMPLES)
    add_subdirectory(examples/)
endif()
