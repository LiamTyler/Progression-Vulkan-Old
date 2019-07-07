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
    core/input.cpp
    core/time.cpp
    core/transform.cpp
    core/window.cpp
    #core/scene.cpp
    core/bounding_box.cpp
    core/frustum.cpp

    #graphics/lights.cpp
    #graphics/render_component.cpp
    #graphics/render_system.cpp
    #graphics/model_render_component.cpp
    #graphics/skybox.cpp
    #graphics/shadow_map.cpp
    graphics/graphics_api.cpp

    resource/resource.cpp
    resource/resource_manager.cpp
    resource/image.cpp
    resource/material.cpp
    resource/mesh.cpp
    resource/model.cpp
    resource/shader.cpp
    resource/texture2D.cpp

    #components/user_camera_component.cpp

    utils/logger.cpp

    ${PROGRESSION_DIR}/ext/tinyobjloader/tiny_obj_loader.cc
    ${PROGRESSION_DIR}/ext/memory_map/MemoryMapped.cpp    
    ${PROGRESSION_DIR}/ext/glad/glad.c

    progression.cpp
)

set(
    HEADERS
    core/camera.hpp
    core/common.hpp
    core/component.hpp
    core/config.hpp
    core/game_object.hpp
    core/input.hpp
    core/input_types.hpp
    core/time.hpp
    core/transform.hpp
    core/window.hpp
    #core/scene.hpp
    core/bounding_box.hpp
    core/frustum.hpp
    
    #graphics/lights.hpp
    #graphics/render_component.hpp
    #graphics/render_system.hpp
    #graphics/model_render_component.hpp
    #graphics/skybox.hpp
    #graphics/shadow_map.hpp
    graphics/graphics_api.hpp

    resource/resource_manager.hpp
    resource/resource.hpp
    resource/image.hpp
    resource/material.hpp
    resource/mesh.hpp
    resource/model.hpp
    resource/shader.hpp
    resource/texture2D.hpp

    #components/user_camera_component.hpp
    
    utils/noncopyable.hpp
    utils/logger.hpp
    utils/random.hpp
    
    ${PROGRESSION_DIR}/ext/tinyobjloader/tiny_obj_loader.h
    ${PROGRESSION_DIR}/ext/stb_image/stb_image.h
    ${PROGRESSION_DIR}/ext/stb_image/stb_image_write.h
    ${PROGRESSION_DIR}/ext/cpptoml.h
    ${PROGRESSION_DIR}/ext/glad/glad.h
    
    ${PROGRESSION_DIR}/ext/memory_map/MemoryMapped.h

    #progression.hpp
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
