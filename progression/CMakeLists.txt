cmake_minimum_required (VERSION 3.0)

project(Progression-lib)

include(Progression-base)

set(PG_AUDIO_ENABLED 0)
if (PROGRESSION_AUDIO)
    set(PG_AUDIO_ENABLED 1)
endif()

configure_file(${PROGRESSION_DIR}/cmake/platform_defines.hpp.in ../../progression/core/platform_defines.hpp)

set(
    SRCS
    core/bounding_box.cpp
    core/camera.cpp
    core/common.cpp
    core/config.cpp
    core/ecs.cpp
    core/frustum.cpp
    core/input.cpp
    core/scene.cpp
    core/time.cpp
    core/transform.cpp
    core/window.cpp

    graphics/graphics_api.cpp
    graphics/pg_to_opengl_types.cpp
    graphics/lights.cpp
    graphics/render_system.cpp
    #graphics/shadow_map.cpp
    #graphics/skybox.cpp

    resource/image.cpp
    resource/material.cpp
    resource/mesh.cpp
    resource/model.cpp
    resource/resource.cpp
    resource/resource_manager.cpp
    resource/shader.cpp
    resource/texture.cpp
    resource/converters/converter.cpp
    resource/converters/material_converter.cpp
    resource/converters/model_converter.cpp
    resource/converters/shader_converter.cpp
    resource/converters/texture_converter.cpp
    resource/converters/fastfile_converter.cpp

    components/model_renderer_component.cpp
    #components/user_camera_component.cpp

    utils/logger.cpp
    utils/random.cpp
    utils/serialize.cpp
    utils/timestamp.cpp

    ${PROGRESSION_DIR}/ext/glad/src/glad.c
    ${PROGRESSION_DIR}/ext/memory_map/MemoryMapped.cpp    
    ${PROGRESSION_DIR}/ext/tinyobjloader/tiny_obj_loader.cc

    progression.cpp
)

set(
    HEADERS
    core/bounding_box.hpp
    core/camera.hpp
    core/common.hpp
    core/config.hpp
    core/ecs.hpp
    core/frustum.hpp
    core/input.hpp
    core/input_types.hpp
    core/scene.hpp
    core/time.hpp
    core/transform.hpp
    core/window.hpp
    
    graphics/graphics_api.hpp
    graphics/pg_to_opengl_types.hpp
    graphics/lights.hpp
    graphics/render_system.hpp
    #graphics/shadow_map.hpp
    #graphics/skybox.hpp

    resource/resource_manager.hpp
    resource/resource.hpp
    resource/image.hpp
    resource/material.hpp
    resource/mesh.hpp
    resource/model.hpp
    resource/shader.hpp
    resource/texture.hpp
    resource/converters/converter.hpp
    resource/converters/material_converter.hpp
    resource/converters/model_converter.hpp
    resource/converters/shader_converter.hpp
    resource/converters/texture_converter.hpp
    resource/converters/fastfile_converter.hpp

    components/model_renderer_component.hpp
    #components/user_camera_component.hpp

    utils/fileIO.hpp
    utils/logger.hpp
    utils/noncopyable.hpp
    utils/random.hpp
    utils/serialize.hpp
    utils/timestamp.hpp
    
    ${PROGRESSION_DIR}/ext/cpptoml.h
    ${PROGRESSION_DIR}/ext/glad/include/glad/glad.h
    ${PROGRESSION_DIR}/ext/glad/include/KHR/khrplatform.h
    ${PROGRESSION_DIR}/ext/memory_map/MemoryMapped.h
    ${PROGRESSION_DIR}/ext/stb_image/stb_image.h
    ${PROGRESSION_DIR}/ext/stb_image/stb_image_write.h
    ${PROGRESSION_DIR}/ext/tinyobjloader/tiny_obj_loader.h

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
