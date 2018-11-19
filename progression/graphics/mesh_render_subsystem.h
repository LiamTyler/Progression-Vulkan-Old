#pragma once

#include "graphics/render_subsystem.h"
#include "graphics/mesh_render_component.h"
#include <vector>

namespace Progression {

    class MeshRenderSubSystem : public RenderSubSystem {
    public:
        MeshRenderSubSystem();
        virtual ~MeshRenderSubSystem() = default;

        virtual void AddRenderComponent(RenderComponent* rc);
        virtual void RemoveRenderComponent(RenderComponent* rc);

        virtual void Render(Scene* scene, Camera& camera);
    
    private:
        std::vector<MeshRenderer*> meshRenderers;
        std::unordered_map<Mesh*, GLuint> vaos_;
    };

} // namespace Progression
