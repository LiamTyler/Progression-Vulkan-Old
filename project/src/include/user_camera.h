#pragma once

#include "include/camera.h"

class UserCamera : public Camera {
    public:
        UserCamera();
        UserCamera(Transform t);
        UserCamera(Transform t, float ms, float ts);

        void Update(float dt);
        void Rotate(glm::vec3 r);

        glm::vec3 velocity;
        float moveSpeed;
        float turnSpeed;
};
