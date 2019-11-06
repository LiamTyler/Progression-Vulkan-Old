#pragma once

#include "core/animation_system.hpp"
#include "core/assert.hpp"
#include "core/camera.hpp"
#include "core/config.hpp"
#include "core/platform_defines.hpp"
#include "core/ecs.hpp"
#include "core/frustum.hpp"
#include "core/input.hpp"
#include "core/input_types.hpp"
#include "core/lua.hpp"
#include "core/math.hpp"
#include "core/scene.hpp"
#include "core/time.hpp"
#include "core/window.hpp"

#include "graphics/graphics_api.hpp"
#include "graphics/lights.hpp"
#include "graphics/render_system.hpp"
#include "graphics/texture_manager.hpp"
#include "graphics/vulkan.hpp"

#include "resource/image.hpp"
#include "resource/material.hpp"
#include "resource/mesh.hpp"
#include "resource/model.hpp"
#include "resource/resource_manager.hpp"
#include "resource/script.hpp"
#include "resource/shader.hpp"
#include "resource/skinned_model.hpp"
#include "utils/logger.hpp"
#include "utils/random.hpp"

#include "components/entity_metadata.hpp"
#include "components/model_renderer.hpp"
#include "components/skinned_renderer.hpp"
#include "components/script_component.hpp"
#include "components/transform.hpp"

namespace PG = Progression;

namespace Progression
{

extern bool g_engineShutdown;
extern bool g_converterMode;

bool EngineInitialize( std::string config_name = "" );

void EngineQuit();

} // namespace Progression
