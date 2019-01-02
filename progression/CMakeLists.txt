cmake_minimum_required (VERSION 3.0)

project(Progression-lib)

include(Progression-base)

set(PG_AUDIO_ENABLED 0)
if (PROGRESSION_AUDIO)
    set(PG_AUDIO_ENABLED 1)
endif()

configure_file(${PROGRESSION_DIR}/cmake/configuration.h.in ../../progression/core/configuration.hpp)

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

    graphics/lights.cpp
    graphics/texture.cpp
    
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
    core/camera.hpp
    core/common.hpp
    core/component.hpp
    core/config.hpp
    core/game_object.hpp
    core/image.hpp
    core/input.hpp
    core/input_types.hpp
    core/time.hpp
    core/transform.hpp
    core/window.hpp
    core/scene.hpp
    core/resource_manager.hpp
    core/bounding_box.hpp
    core/frustum.hpp
    
    graphics/lights.hpp
    graphics/texture.hpp

    graphics/material.hpp
    graphics/mesh.hpp
    graphics/model.hpp
    graphics/shader.hpp
    graphics/render_component.hpp
    graphics/render_system.hpp
    graphics/render_subsystem.hpp
    graphics/mesh_render_subsystem.hpp
    graphics/mesh_render_component.hpp
    graphics/model_render_component.hpp
    graphics/skybox.hpp
    graphics/graphics_api.hpp

    components/user_camera_component.hpp
    
    utils/noncopyable.hpp
    
    ${CMAKE_SOURCE_DIR}/ext/tinyobjloader/tiny_obj_loader.h
    ${CMAKE_SOURCE_DIR}/ext/stb_image/stb_image.h
    ${CMAKE_SOURCE_DIR}/ext/stb_image/stb_image_write.h
    ${CMAKE_SOURCE_DIR}/ext/cpptoml.h
    ${CMAKE_SOURCE_DIR}/ext/glad/glad.h
    
    ${CMAKE_SOURCE_DIR}/ext/memory_map/MemoryMapped.h

    progression.hpp
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
