#include "graphics/render_component.hpp"

namespace Progression {

    RenderComponent::RenderComponent(GameObject* obj, bool active_) :
        Component(obj),
        active(active_),
        visible(true)
    {
    }

} // namespace Progression