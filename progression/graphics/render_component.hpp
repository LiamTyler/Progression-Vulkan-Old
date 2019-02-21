#pragma once

#include "core/component.hpp"

namespace Progression {

    class RenderComponent : public Component {
    public:
        RenderComponent(GameObject* obj);
        virtual ~RenderComponent() = default;
        virtual void Start() = 0;
        virtual void Update() = 0;
        virtual void Stop() = 0;

        bool visible = true;
    };

} // namespace Progression
