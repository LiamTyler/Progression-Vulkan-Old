#pragma once

#include "core/camera.hpp"
#include "core/common.hpp"
#include "core/component.hpp"
#include "core/config.hpp"
#include "core/game_object.hpp"
#include "core/image.hpp"
#include "core/input.hpp"
#include "core/input_types.hpp"
#include "core/time.hpp"
#include "core/transform.hpp"
#include "core/window.hpp"
#include "core/resource_manager.hpp"
#include "core/scene.hpp"
#include "core/bounding_box.hpp"
#include "core/frustum.hpp"
#include "core/configuration.hpp"

#include "graphics/material.hpp"
#include "graphics/mesh.hpp"
#include "graphics/model.hpp"
#include "graphics/shader.hpp"
#include "graphics/render_system.hpp"
#include "graphics/render_component.hpp"
#include "graphics/model_render_component.hpp"
#include "graphics/skybox.hpp"
#include "graphics/shadow_map.hpp"
#include "graphics/graphics_api.hpp"

#include "graphics/lights.hpp"
#include "graphics/texture2D.hpp"
#include "utils/logger.hpp"
#include "utils/random.hpp"

#include "components/user_camera_component.hpp"

#if PG_AUDIO_ENABLED
    #include "audio/audio_system.hpp"
    #include "audio/audio_file.hpp"
#endif

namespace PG = Progression;

namespace Progression {

    extern bool EngineShutdown;

    void EngineInitialize(std::string config_name = "");

    void EngineQuit();

} // namespace Progression
