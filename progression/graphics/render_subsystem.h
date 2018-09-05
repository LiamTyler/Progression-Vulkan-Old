#pragma once

#include "core/camera.h"

namespace Progression {

    class RenderComponent;
    class Scene;

    class RenderSubSystem {
    public:
        RenderSubSystem() = default;
        virtual ~RenderSubSystem() = default;

        virtual void AddRenderComponent(RenderComponent* rc) = 0;
        virtual void RemoveRenderComponent(RenderComponent* rc) = 0;

        virtual void Render(Scene* scene) = 0;

    };

} // namespace Progression