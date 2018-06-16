#include "include/user_camera.h"

UserCamera::UserCamera(const Transform& t, float ms, float ts) :
	Camera(t),
	moveSpeed(ms),
    turnSpeed(ts),
    velocity(glm::vec3(0)) {
}

void UserCamera::Update(float dt) {
    transform.position += dt * moveSpeed *
        (velocity.x * currRight_ + velocity.y * currUp_ + velocity.z * currDir_);
    UpdateViewMatrix();
}

void UserCamera::Rotate(const glm::vec3& r) {
	float xLimit = glm::radians(85.0f);
	transform.rotation += r * turnSpeed;
	transform.rotation.x = std::fmax(-xLimit, std::fmin(xLimit, transform.rotation.x));
    UpdateOrientationVectors();
}
