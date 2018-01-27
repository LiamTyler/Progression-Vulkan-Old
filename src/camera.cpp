#include "include/camera.h"

Camera::Camera() : Camera(
        vec3(0, 0, 0),
        vec3(0, 0, -1),
        vec3(0, 1, 0)) {}

Camera::Camera(vec3 pos, vec3 dir, vec3 up) : Camera(pos, dir, up, 1, 0.005) {}

Camera::Camera(vec3 pos, vec3 dir, vec3 up, float sLinear, float sAngular) {
    pos_ = pos;
    initialDir_ = dir;
    initialUp_ = up;
    initialRight_ = cross(initialDir_, initialUp_);
    currDir_ = initialDir_;
    currUp_ = initialUp_;
    currRight_ = initialRight_;
    speedLinear_ = sLinear;
    speedAngular_ = sAngular;
    Setup();
}

void Camera::Setup() {
    SetProjection(radians(45.0f), 4.0f/3.0f, 0.1f, 100.0f);
}

void Camera::SetProjection(float fov, float aspect, float near_p, float far_p) {
    projection_ = perspective(fov, aspect, near_p, far_p);
}

void Camera::UpdateViewMatrix() {
    view_ = lookAt(pos_, pos_ + currDir_, currUp_);
}

void Camera::Update(float dt) {
    pos_ = pos_ + dt * speedLinear_ * (vel_.z*currDir_ + vel_.x*currRight_);
    UpdateViewMatrix();
}

void Camera::UpdateAxis() {
    mat4 rot(1);
    rot = rotate(rot, rotation_.y, initialUp_);
    rot = rotate(rot, rotation_.x, initialRight_);
    currDir_ = vec3(rot*vec4(initialDir_, 0));
    currUp_ = vec3(rot*vec4(initialUp_, 0));
    currRight_ = cross(currDir_, currUp_);
}

void Camera::RotateY(float y) {
    rotation_.y += y*speedAngular_;
}

void Camera::RotateX(float x) {
    float cap = radians(85.0f);
    rotation_.x = fmin(cap, fmax(-cap, rotation_.x + x * speedAngular_));
}
