#include "include/camera.h"

Camera::Camera(const Transform& t, float fov, float a, float np, float fp) :
	transform(t),
	fieldOfView_(fov),
	aspectRatio_(a),
	nearPlane_(np),
	farPlane_(fp)
{
	UpdateOrientationVectors();
	UpdateProjectionMatrix();
	UpdateViewMatrix();
}

void Camera::UpdateOrientationVectors() {
    glm::mat4 rot(1);
    rot = glm::rotate(rot, transform.rotation.y, glm::vec3(0, 1, 0));
    rot = glm::rotate(rot, transform.rotation.x, glm::vec3(1, 0, 0));
    currDir_ = glm::vec3(rot * glm::vec4(0, 0, -1, 0));
    currUp_ = glm::vec3(rot * glm::vec4(0, 1, 0, 0));
    currRight_ = glm::cross(currDir_, currUp_);
}

void Camera::UpdateViewMatrix() {
    viewMatrix_ = glm::lookAt(
            transform.position,
            transform.position + currDir_,
            currUp_);
}

void Camera::UpdateProjectionMatrix() {
    projectionMatrix_ = glm::perspective(fieldOfView_, aspectRatio_, nearPlane_, farPlane_);
}

glm::mat4 Camera::GetV() const { return viewMatrix_; }
glm::mat4 Camera::GetP() const { return projectionMatrix_; }
float Camera::GetFOV() const { return fieldOfView_; }
float Camera::GetAspectRatio() const { return aspectRatio_; }
float Camera::GetNearPlane() const { return nearPlane_; }
float Camera::GetFarPlane() const { return farPlane_; }
glm::vec3 Camera::GetForwardDir() const { return currDir_; }
glm::vec3 Camera::GetUpDir() const { return currUp_; }
glm::vec3 Camera::GetRightDir() const { return currRight_; }

void Camera::SetFOV(float f) {
    fieldOfView_ = f;
    UpdateProjectionMatrix();
}

void Camera::SetAspectRatio(float a) {
    aspectRatio_ = a;
    UpdateProjectionMatrix();
}

void Camera::SetNearPlane(float p) {
    nearPlane_ = p;
    UpdateProjectionMatrix();
}

void Camera::SetFarPlane(float p) {
    farPlane_ = p;
    UpdateProjectionMatrix();
}
