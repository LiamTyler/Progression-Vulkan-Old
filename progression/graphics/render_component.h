#pragma once

#include "core/component.h"

namespace Progression {

    class RenderComponent : public Component {
    public:
        RenderComponent(GameObject* obj, bool active = true);
        virtual ~RenderComponent() = default;
        virtual void Start() = 0;
        virtual void Update() = 0;
        virtual void Stop() = 0;

        bool active;
        bool visible;
    };

} // namespace Progression