#pragma once

#include "core/component.hpp"
#include "core/camera.hpp"

namespace Progression {

    class UserCameraComponent : public Component {
    public:
        UserCameraComponent(Camera* camera, float ms = 3, float ts = .002, float maxAngle = 85);
        virtual ~UserCameraComponent() = default;
        virtual void Start();
        virtual void Update();
        virtual void Stop();

        float moveSpeed;
        float turnSpeed;
        float maxAngle;
        glm::vec3 velocity;
    };
    
} // namespace Progression
