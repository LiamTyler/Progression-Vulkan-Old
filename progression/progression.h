#pragma once

#include "core/camera.h"
#include "core/common.h"
#include "core/component.h"
#include "core/config.h"
#include "core/game_object.h"
#include "core/image.h"
#include "core/input.h"
#include "core/input_types.h"
#include "core/time.h"
#include "core/transform.h"
#include "core/window.h"
#include "core/resource_manager.h"
#include "core/scene.h"
#include "core/bounding_box.h"
#include "core/frustum.h"
#include "core/configuration.h"

#include "graphics/material.h"
#include "graphics/mesh.h"
#include "graphics/model.h"
#include "graphics/shader.h"
#include "graphics/render_system.h"
#include "graphics/render_subsystem.h"
#include "graphics/render_component.h"
#include "graphics/model_render_component.h"
#include "graphics/mesh_render_subsystem.h"
#include "graphics/mesh_render_component.h"
#include "graphics/skybox.h"
#include "graphics/graphics_api.h"

#include "types/lights.h"
#include "types/texture.h"

#include "components/user_camera_component.h"

#if PG_AUDIO_ENABLED
    #include "audio/audio_system.hpp"
    #include "audio/audio_file.hpp"
#endif

namespace PG = Progression;

namespace Progression {

    bool EngineShutdown = false;

    inline void EngineInitialize(const config::Config& conf) {
        Window::Init(conf);
        Time::Init(conf);
        Input::Init(conf);
        ResourceManager::Init(conf);
        RenderSystem::Init(conf);
        #if PG_AUDIO_ENABLED
            AudioSystem::Init(conf);
        #endif
    }

    inline void EngineQuit() {
        #if PG_AUDIO_ENABLED
            AudioSystem::Free();
        #endif
        RenderSystem::Free();
        ResourceManager::Free();
        Input::Free();
        Time::Free();
        Window::Free();
    }

} // namespace Progression
