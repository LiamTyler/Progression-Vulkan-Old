cmake_minimum_required (VERSION 3.0)

project(Progression-lib)

include(Progression-base)

set(
    SRCS
    src/background.cpp
    src/camera.cpp
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
    src/input.cpp

    core/time.cpp

    utils/config.cpp
    )

set (
    HEADERS
    src/include/background.h
    src/include/camera.h
    src/include/component.h
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
    src/include/input.h
    src/include/input_types.h

    core/time.h

    utils/config.h

    progression.h
    )
	

add_library(progression STATIC ${SRCS} ${HEADERS})

set_target_properties(
    progression
    PROPERTIES
    DEBUG_POSTFIX _debug
    )

# link in the static libs
target_link_libraries(progression ${PROGRESSION_LIBS})
