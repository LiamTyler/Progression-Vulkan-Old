#pragma once

#include "graphics/render_subsystem.h"
#include "graphics/mesh_renderer.h"

namespace Progression {

    class MeshRenderSubSystem : public RenderSubSystem {
        MeshRenderSubSystem();
        virtual ~MeshRenderSubSystem() = default;

        virtual void AddRenderComponent(RenderComponent* rc);
        virtual void RemoveRenderComponent(RenderComponent* rc);

        virtual void Render(Scene* scene);

        std::vector<MeshRenderer*> meshRenderers;
    };

} // namespace Progression