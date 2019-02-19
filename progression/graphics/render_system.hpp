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
    bool Init(const config::Config& config);

    /**
     * \brief Free all of the resources used by the rendering engine and shutdown.
     */
    void Free();

    /**
     * \brief Will render the given scene.
     */
    void render(Scene* scene);

} } // namespace Progression::RenderSystem
