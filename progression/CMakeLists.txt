cmake_minimum_required (VERSION 3.0)

project(Progression-lib)

include(Progression-base)


configure_file(${PROGRESSION_DIR}/cmake/platform_defines.hpp.in ${PROGRESSION_DIR}/progression/core/platform_defines.hpp)

set(
    GRAPHICS
    graphics/graphics_api.cpp
    graphics/render_system.cpp

    graphics/graphics_api.hpp
    graphics/pg_to_opengl_types.hpp
    graphics/lights.hpp
    graphics/render_system.hpp
)

set(
	CORE
	core/bounding_box.cpp
    core/camera.cpp
    core/config.cpp
    core/ecs.cpp
    core/frustum.cpp
    core/input.cpp
    core/scene.cpp
    core/time.cpp
    core/transform.cpp
    core/window.cpp
	
	core/assert.hpp
	core/unused.hpp
	core/bounding_box.hpp
    core/camera.hpp
    core/common.hpp
    core/config.hpp
    core/ecs.hpp
    core/frustum.hpp
    core/feature_defines.hpp
    core/input.hpp
    core/input_types.hpp
    core/platform_defines.hpp
    core/scene.hpp
    core/time.hpp
    core/transform.hpp
    core/using_defines.hpp
    core/window.hpp
)

set(
	RESOURCE
	resource/image.cpp
    resource/material.cpp
    resource/mesh.cpp
    resource/model.cpp
    resource/resource_manager.cpp
    resource/shader.cpp
    resource/texture.cpp
    
	
	resource/resource_manager.hpp
    resource/resource.hpp
    resource/image.hpp
    resource/material.hpp
    resource/mesh.hpp
    resource/model.hpp
    resource/shader.hpp
    resource/texture.hpp
)

set(
	RESOURCE_CONVERTERS
	resource/converters/converter.cpp
    resource/converters/material_converter.cpp
    resource/converters/model_converter.cpp
    resource/converters/shader_converter.cpp
    resource/converters/texture_converter.cpp
    resource/converters/fastfile_converter.cpp
	
	resource/converters/converter.hpp
    resource/converters/material_converter.hpp
    resource/converters/model_converter.hpp
    resource/converters/shader_converter.hpp
    resource/converters/texture_converter.hpp
    resource/converters/fastfile_converter.hpp
)	

set(
	EXTERNALS
	../ext/getopt/getopt.c
    ../ext/glad/src/glad.c
    ../ext/memory_map/MemoryMapped.cpp    
    ../ext/tinyobjloader/tiny_obj_loader.cc
	
	../ext/cpptoml.h
    ../ext/getopt/getopt.h
    ../ext/glad/include/glad/glad.h
    ../ext/glad/include/KHR/khrplatform.h
    ../ext/memory_map/MemoryMapped.h
    ../ext/stb_image/stb_image.h
    ../ext/stb_image/stb_image_write.h
    ../ext/tinyobjloader/tiny_obj_loader.h
)

set(
	UTILS
	utils/logger.cpp
    utils/random.cpp
    utils/timestamp.cpp
	
	utils/fileIO.hpp
    utils/logger.hpp
    utils/noncopyable.hpp
    utils/random.hpp
    utils/serialize.hpp
    utils/timestamp.hpp
)

set(
	COMPONENTS
	components/model_renderer_component.cpp
	
	components/model_renderer_component.hpp
)

set(
	OTHER
    progression.cpp
)

set(
	ALL_FILES
	${GRAPHICS}
	${CORE}
	${RESOURCE}
	${RESOURCE_CONVERTERS}
	${UTILS}
	${COMPONENTS}
	${EXTERNALS}
	${OTHER}
)

# Needs cmake 3.12.4
#list(TRANSFORM ALL_FILES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)

SET(listVar "")
FOREACH(f ${ALL_FILES})
    LIST(APPEND listVar "${CMAKE_CURRENT_SOURCE_DIR}/${f}")
ENDFOREACH(f)
set(ALL_FILES ${listVar})

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ALL_FILES})

add_library(progression STATIC ${ALL_FILES})

if(MSVC)
    target_compile_options(progression PRIVATE "/MP")
endif()

#set_target_properties(
#    progression
#    PROPERTIES
#    DEBUG_POSTFIX _debug
#    DEBUG_POSTFIX _release
#    SHIP_POSTFIX _ship
#)

# link in the static libs
target_link_libraries(progression ${PROGRESSION_LIBS})
