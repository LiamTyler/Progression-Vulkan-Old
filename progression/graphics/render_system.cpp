#include "graphics/render_system.h"
#include "graphics/render_subsystem.h"

namespace Progression {

    void RenderSystem::Init(const config::Config& config) {

    }

    void RenderSystem::Free() {
        for (auto& subsys : subSystems_)
            delete subsys.second;
    }

    void RenderSystem::Render(Scene* scene) {
        
    }

} // namespace Progression