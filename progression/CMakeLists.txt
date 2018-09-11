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
    core/scene.cpp
    core/resource_manager.cpp

    types/lights.cpp
    types/texture.cpp
    
    graphics/material.cpp
    graphics/mesh.cpp
    graphics/model.cpp
    graphics/shader.cpp
    graphics/render_component.cpp
    graphics/render_system.cpp
    graphics/render_subsystem.cpp
    graphics/mesh_render_subsystem.cpp
    graphics/mesh_render_component.cpp
    graphics/model_render_component.cpp
    graphics/skybox.cpp

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
    core/scene.h
    core/resource_manager.h
    
    types/lights.h
    types/texture.h

    graphics/material.h
    graphics/mesh.h
    graphics/model.h
    graphics/shader.h
    graphics/render_component.h
    graphics/render_system.h
    graphics/render_subsystem.h
    graphics/mesh_render_subsystem.h
    graphics/mesh_render_component.h
    graphics/model_render_component.h
    graphics/skybox.h

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
