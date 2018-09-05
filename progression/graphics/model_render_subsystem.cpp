#include "graphics/model_render_subsystem.h"

namespace Progression {

    MeshRenderSubSystem::MeshRenderSubSystem() {

    }

    void MeshRenderSubSystem::AddRenderComponent(RenderComponent* rc) {
        meshRenderers.push_back(static_cast<MeshRenderer*>(rc));
    }
    
    // TODO: Implement
    void MeshRenderSubSystem::RemoveRenderComponent(RenderComponent* rc) {

    }

    void MeshRenderSubSystem::Render(Scene* scene) {

    }

} // namespace Progression