cmake_minimum_required (VERSION 3.0)

project(Progression-lib)

include(Progression-base)

set(PG_AUDIO_ENABLED 0)
if (PROGRESSION_AUDIO)
    set(PG_AUDIO_ENABLED 1)
endif()

configure_file(${PROGRESSION_DIR}/cmake/configuration.h.in ../../progression/core/configuration.h)

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
    core/bounding_box.cpp
    core/frustum.cpp

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
    graphics/graphics_api.cpp

    components/user_camera_component.cpp

    ${CMAKE_SOURCE_DIR}/ext/tinyobjloader/tiny_obj_loader.cc
    ${CMAKE_SOURCE_DIR}/ext/memory_map/MemoryMapped.cpp    
    ${CMAKE_SOURCE_DIR}/ext/glad/glad.c
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
    core/bounding_box.h
    core/frustum.h
    
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
    graphics/graphics_api.h

    components/user_camera_component.h
    
    ${CMAKE_SOURCE_DIR}/ext/tinyobjloader/tiny_obj_loader.h
    ${CMAKE_SOURCE_DIR}/ext/stb_image/stb_image.h
    ${CMAKE_SOURCE_DIR}/ext/stb_image/stb_image_write.h
    ${CMAKE_SOURCE_DIR}/ext/cpptoml.h
    ${CMAKE_SOURCE_DIR}/ext/glad/glad.h
    
    ${CMAKE_SOURCE_DIR}/ext/memory_map/MemoryMapped.h

    progression.h
)

set(AUDIO_SRCS
    audio/audio_file.cpp
    audio/audio_source.cpp
    audio/audio_system.cpp
)

set(AUDIO_INCLUDES
    audio/audio_file.hpp
    audio/audio_source.hpp
    audio/audio_system.hpp
)

if (PROGRESSION_AUDIO)
    set(SRCS ${SRCS} ${AUDIO_SRCS})
    set(HEADERS ${HEADERS} ${AUDIO_INCLUDES})
endif()

add_library(progression STATIC ${SRCS} ${HEADERS})

if(MSVC)
    target_compile_options(progression PRIVATE "/MP")
endif()

set_target_properties(
    progression
    PROPERTIES
    DEBUG_POSTFIX _debug
)

# link in the static libs
target_link_libraries(progression ${PROGRESSION_LIBS})
