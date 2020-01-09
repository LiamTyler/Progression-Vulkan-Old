cmake_minimum_required (VERSION 3.0)

project(Progression-lib)

include(Progression-base)

configure_file(${PROGRESSION_DIR}/cmake/platform_defines.hpp.in ${PROGRESSION_DIR}/progression/core/platform_defines.hpp)

set(
    GRAPHICS
    #graphics/graphics_api.cpp
    graphics/debug_marker.cpp
    graphics/render_system.cpp
    graphics/shadow_map.cpp
    graphics/texture_manager.cpp
    graphics/vulkan.cpp

    graphics/debug_marker.hpp
    graphics/graphics_api.hpp
    graphics/lights.hpp
    graphics/pg_to_vulkan_types.hpp
    graphics/render_system.hpp
    graphics/shadow_map.hpp
    graphics/texture_manager.hpp
    graphics/vulkan.hpp
    
    graphics/graphics_api/buffer.cpp
    graphics/graphics_api/buffer.hpp
    graphics/graphics_api/command_buffer.cpp
    graphics/graphics_api/command_buffer.hpp
    graphics/graphics_api/descriptor.cpp
    graphics/graphics_api/descriptor.hpp
    graphics/graphics_api/device.cpp
    graphics/graphics_api/device.hpp
    graphics/graphics_api/framebuffer.cpp
    graphics/graphics_api/framebuffer.hpp
    graphics/graphics_api/pipeline.cpp
    graphics/graphics_api/pipeline.hpp
    graphics/graphics_api/profiling.cpp
    graphics/graphics_api/profiling.hpp
    graphics/graphics_api/render_pass.cpp
    graphics/graphics_api/render_pass.hpp
    graphics/graphics_api/sampler.cpp
    graphics/graphics_api/sampler.hpp
    graphics/graphics_api/synchronization.cpp
    graphics/graphics_api/synchronization.hpp
    graphics/graphics_api/texture.cpp
    graphics/graphics_api/texture.hpp
    graphics/graphics_api/ui.cpp
    graphics/graphics_api/ui.hpp
    graphics/graphics_api/vertex_descriptor.cpp
    graphics/graphics_api/vertex_descriptor.hpp
    
    graphics/shader_c_shared/defines.h
    graphics/shader_c_shared/lights.h
    graphics/shader_c_shared/structs.h
)

set(
	CORE
	core/animation_system.cpp
	core/bounding_box.cpp
    core/camera.cpp
    core/config.cpp
    core/ecs.cpp
    core/frustum.cpp
    core/input.cpp
    core/lua.cpp
    core/math.cpp
    core/scene.cpp
    core/time.cpp
    core/window.cpp
	
    core/animation_system.hpp
	core/assert.hpp
	core/bounding_box.hpp
    core/camera.hpp
    core/config.hpp
    core/core_defines.hpp
    core/ecs.hpp
    core/frustum.hpp
    core/feature_defines.hpp
    core/input.hpp
    core/input_types.hpp
    core/lua.hpp
    core/math.hpp
    core/platform_defines.hpp
    core/scene.hpp
    core/time.hpp
    core/window.hpp
)

set(
	RESOURCE
	resource/image.cpp
    resource/material.cpp
    resource/model.cpp
    resource/resource_manager.cpp
    resource/script.cpp
    resource/shader.cpp
    
    resource/resource.hpp
    resource/resource_manager.hpp
    resource/resource_version_numbers.hpp
    resource/image.hpp
    resource/material.hpp
    resource/model.hpp
    resource/script.hpp
    resource/shader.hpp
)

set(
	RESOURCE_CONVERTERS
    resource/converters/converter.cpp
    resource/converters/fastfile_converter.cpp
    resource/converters/image_converter.cpp
    resource/converters/material_converter.cpp
    resource/converters/model_converter.cpp
    resource/converters/script_converter.cpp
    resource/converters/shader_converter.cpp
	
	resource/converters/converter.hpp
    resource/converters/fastfile_converter.hpp
    resource/converters/image_converter.hpp
    resource/converters/material_converter.hpp
    resource/converters/model_converter.hpp
    resource/converters/script_converter.hpp
    resource/converters/shader_converter.hpp
)

set(
	EXTERNALS
	../ext/getopt/getopt.c
    ../ext/memory_map/MemoryMapped.cpp    
	
	../ext/cpptoml.h
    ../ext/getopt/getopt.h
    ../ext/memory_map/MemoryMapped.h
    ../ext/stb_image/stb_image.h
    ../ext/stb_image/stb_image_write.h

    ../ext/SPIRV-Reflect/spirv_reflect.h
    ../ext/SPIRV-Reflect/spirv_reflect.c
    ../ext/SPIRV-Reflect/common/output_stream.cpp
    ../ext/SPIRV-Reflect/common/output_stream.h
    
    ../ext/imgui/imgui.h
    ../ext/imgui/imgui.cpp
    ../ext/imgui/imgui_widgets.cpp
    ../ext/imgui/imgui_draw.cpp
    ../ext/imgui/imgui_demo.cpp
)

set(
	UTILS
	utils/json_parsing.cpp
	utils/logger.cpp
    utils/random.cpp
    utils/string.cpp
    utils/timestamp.cpp
	
	utils/fileIO.hpp
	utils/json_parsing.hpp
    utils/logger.hpp
    utils/noncopyable.hpp
    utils/random.hpp
    utils/serialize.hpp
    utils/string.hpp
    utils/timestamp.hpp
    utils/type_name.hpp
)

set(
	COMPONENTS
	components/animation_component.cpp
	components/animation_component.hpp
	components/entity_metadata.hpp
	components/factory.cpp
	components/factory.hpp
	components/model_renderer.hpp
	components/script_component.cpp
    components/script_component.hpp
    components/skinned_renderer.hpp
	components/transform.cpp
	components/transform.hpp
)

set(
	OTHER
    progression.cpp
    progression.hpp
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

include_directories(
    ${CMAKE_SOURCE_DIR}/ext/SPIRV-Reflect/
    ${CMAKE_SOURCE_DIR}/ext/SPIRV-Reflect/examples
    ${CMAKE_SOURCE_DIR}/ext/SPIRV-Reflect/common
)

SET(listVar "")
FOREACH(f ${ALL_FILES})
    LIST(APPEND listVar "${CMAKE_CURRENT_SOURCE_DIR}/${f}")
ENDFOREACH(f)
set(ALL_FILES ${listVar})

source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${ALL_FILES})

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_library(progression STATIC ${ALL_FILES})

if(MSVC)
    target_compile_options(progression PRIVATE "/MP")
endif()

SET_TARGET_POSTFIX( progression )

# link in the static libs
target_link_libraries(progression ${PROGRESSION_LIBS})
