#pragma once

#include "core/camera.hpp"
#include "graphics/shader.hpp"

namespace Progression {

    class RenderComponent;
    class Scene;

    class RenderSubSystem {
    public:
        virtual void AddRenderComponent(RenderComponent* rc) = 0;
        virtual void RemoveRenderComponent(RenderComponent* rc) = 0;
        virtual void Render(Scene* scene, Camera& camera) = 0;

    protected:
        Shader pipelineShaders[RenderingPipeline::NUM_PIPELINES];
    };

} // namespace Progression
