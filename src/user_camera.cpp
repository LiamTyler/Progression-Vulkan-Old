#include "include/user_camera.h"

UserCamera::UserCamera() : UserCamera(Transform()) {}

UserCamera::UserCamera(Transform t) : UserCamera(t, 3, 0.005) {}

UserCamera::UserCamera(Transform t, float ms, float ts) : Camera(t) {
    moveSpeed = ms;
    turnSpeed = ts;
}

void UserCamera::Update(float dt) {
    transform.position += dt * moveSpeed *
        (velocity.x * currRight + velocity.y * currUp + velocity.z * currDir);
    UpdateViewMatrix();
}

void UserCamera::Rotate(glm::vec3 r) {
    transform.rotation += r * turnSpeed;
    UpdateOrientationVectors();
}
