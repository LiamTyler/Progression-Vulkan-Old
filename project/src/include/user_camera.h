#pragma once

#include "include/camera.h"

class UserCamera : public Camera {
    public:
        UserCamera(
			const Transform& t = Transform(),
			float ms = 3,
			float ts = .002);

        void Update(float dt);
        void Rotate(const glm::vec3& r);

        glm::vec3 velocity;
        float moveSpeed;
        float turnSpeed;
};
