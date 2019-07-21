#pragma once

#include "core/common.hpp"
#include "core/config.hpp"

namespace Progression {

    class Scene;
    
namespace RenderSystem {

    /**
     * \brief Initialze the main rendering engine. This includes the shaders,
     *        GBuffer, etc.
     */
    bool init(const config::Config& config);

    /**
     * \brief Free all of the resources used by the rendering engine and shutdown.
     */
    void shutdown();

    /**
     * \brief Will render the given scene.
     */
    void render(Scene* scene);

} } // namespace Progression::RenderSystem
