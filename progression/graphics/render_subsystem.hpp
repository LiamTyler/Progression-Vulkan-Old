#pragma once

#include "core/camera.hpp"
#include "graphics/shader.hpp"

namespace Progression {

    class RenderComponent;
    class Scene;

    class RenderSubSystem {
    public:
        virtual ~RenderSubSystem() = default;
        virtual void AddRenderComponent(RenderComponent* rc) = 0;
        virtual void RemoveRenderComponent(RenderComponent* rc) = 0;
        virtual void DepthPass(Shader& shader, const glm::mat4& LSM) = 0;
        virtual void Render(const Camera& camera) = 0;

    protected:
        Shader pipelineShaders[RenderingPipeline::NUM_PIPELINES];
    };

} // namespace Progression
