#pragma once

#include "graphics/render_subsystem.hpp"
#include "graphics/mesh_render_component.hpp"
#include <vector>

namespace Progression {

    class MeshRenderSubSystem : public RenderSubSystem {
    public:
        MeshRenderSubSystem();
        virtual ~MeshRenderSubSystem();

        virtual void AddRenderComponent(RenderComponent* rc);
        virtual void RemoveRenderComponent(RenderComponent* rc);

        virtual void Render(Scene* scene, Camera& camera);
    
    private:
        std::vector<MeshRenderer*> meshRenderers;
        std::unordered_map<Mesh*, GLuint> vaos_;
    };

} // namespace Progression
