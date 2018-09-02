cmake_minimum_required (VERSION 3.0)

project(Progression-lib)

include(Progression-base)

set(
    SRCS
    core/camera.cpp
    core/common.cpp
    core/config.cpp
    core/game_object.cpp
    core/image.cpp
    core/input.cpp
    core/time.cpp
    core/transform.cpp
    core/window.cpp
    
    graphics/material.cpp
    graphics/mesh.cpp
    graphics/shader.cpp

    components/user_camera_component.cpp

    ${CMAKE_SOURCE_DIR}/ext/tinyobjloader/tiny_obj_loader.cc
    )

set(
    HEADERS
    core/camera.h
    core/common.h
    core/component.h
    core/config.h
    core/game_object.h
    core/image.h
    core/input.h
    core/input_types.h
    core/time.h
    core/transform.h
    core/window.h
    
    graphics/material.h
    graphics/mesh.h
    graphics/shader.h

    components/user_camera_component.h
    
    ${CMAKE_SOURCE_DIR}/ext/tinyobjloader/tiny_obj_loader.h
    ${CMAKE_SOURCE_DIR}/ext/stb_image/stb_image.h
    ${CMAKE_SOURCE_DIR}/ext/stb_image/stb_image_write.h

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
