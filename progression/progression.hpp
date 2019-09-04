#pragma once

#include "core/camera.hpp"
#include "core/config.hpp"
#include "core/platform_defines.hpp"
#include "core/ecs.hpp"
#include "core/frustum.hpp"
#include "core/input.hpp"
#include "core/input_types.hpp"
#include "core/scene.hpp"
#include "core/time.hpp"
#include "core/transform.hpp"
#include "core/window.hpp"

#include "graphics/graphics_api.hpp"
#include "graphics/render_system.hpp"

#include "graphics/lights.hpp"

#include "resource/image.hpp"
#include "resource/material.hpp"
#include "resource/mesh.hpp"
#include "resource/model.hpp"
#include "resource/resource_manager.hpp"
#include "resource/shader.hpp"
#include "resource/texture.hpp"
#include "utils/logger.hpp"
#include "utils/random.hpp"

#include "components/model_renderer_component.hpp"

namespace PG = Progression;

namespace Progression
{

extern bool EngineShutdown;

void EngineInitialize( std::string config_name = "" );

void EngineQuit();

} // namespace Progression
