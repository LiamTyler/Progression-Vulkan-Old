cmake_minimum_required (VERSION 3.0)

project(OpenGL_Starter-lib)

if (WIN32)

    # Path to main directory of GLEW and SDL2
    set(GLEW_DIR C:/GLEW)
    set(SDL_DIR C:/SDL2)

    # set the include directories for GLEW and SDL2
    include_directories(SYSTEM ${GLEW_DIR}/include ${SDL_DIR}/include)

    # set the directories for GLEW and SDL2 static lib files
    link_directories(${GLEW_DIR}/lib/Release/x64 ${SDL_DIR}/lib/x64)

    # store the location of the DLL's
    set(GLEW_DLL ${GLEW_DIR}/bin/Release/x64/glew32.dll)
    set(SDL_DLL ${SDL_DIR}/lib/x64/SDL2.dll)

    set(
        SRCS
        src/background.cpp
        src/camera.cpp
        src/fps_counter.cpp
        src/game_object.cpp
        src/image.cpp
        src/lights.cpp
        src/material.cpp
        src/mesh.cpp
        src/mesh_renderer.cpp
        src/model.cpp
        src/model_renderer.cpp
        src/OBJ_Loader.cpp
        src/shader.cpp
        src/skybox.cpp
        src/texture.cpp
        src/transform.cpp
        src/user_camera.cpp
        src/utils.cpp
        src/window.cpp
        )

    set (
        HEADERS
        src/include/background.h
        src/include/camera.h
        src/include/component.h
        src/include/fps_counter.h
        src/include/game_object.h
        src/include/image.h
        src/include/lights.h
        src/include/material.h
        src/include/mesh.h
        src/include/mesh_renderer.h
        src/include/model.h
        src/include/model_renderer.h
        src/include/OBJ_Loader.h
        src/include/shader.h
        src/include/skybox.h
        src/include/stb_image.h
        src/include/stb_image_write.h
        src/include/texture.h
        src/include/transform.h
        src/include/user_camera.h
        src/include/utils.h
        src/include/window.h
        )

    add_library(${OGL_STARTER_LIB} STATIC ${SRCS} ${HEADERS})

    set_target_properties(
        ${OGL_STARTER_LIB}
        PROPERTIES
        DEBUG_POSTFIX _debug
        )

    # Tell visual studio to build & run this project instead of ALL_BUILD by default.
    # set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

    # link in the static libs
    target_link_libraries(${OGL_STARTER_LIB} ${EXTERNAL_LIBS})

    SET(fileOut "#pragma once\n\n")
    FOREACH(f ${HEADERS})
        set(fileOut "${fileOut}#include \"${f}\"\n")
    ENDFOREACH(f)

    set(STARTERH ${CMAKE_BINARY_DIR}/include/starter.h)
    message("Starter.h at: ${STARTERH}")

    if (EXISTS ${STARTERH}) 
        file(READ ${STARTERH} fileIn)
        if (NOT ${fileIn} STREQUAL ${fileOut})
            message("rebuilding starter.h")
            file(WRITE ${STARTERH} ${fileOut})
        endif()
    else()
        file(WRITE ${STARTERH} ${fileOut})
    endif()

endif()
