#ifndef SRC_INCLUDE_CAMERA_H_
#define SRC_INCLUDE_CAMERA_H_

#include "include/utils.h"

class Camera {
    public:
        Camera();
        Camera(vec3 pos, vec3 dir, vec3 up);
        Camera(vec3 pos, vec3 dir, vec3 up, float sLinear, float sAngular);
        void Setup();

        void SetProjection(float fov, float aspect, float near_p, float far_p);
        void Update(float dt);
        void UpdateAxis();
        void UpdateViewMatrix();

        void RotateY(float y);
        void RotateX(float x);

        // Getters
        mat4 View() { return view_; }
        mat4 Proj() { return projection_; }
        mat4 VP() { return projection_ * view_; }
        vec3 Vel() { return vel_; }
        vec3 Pos() { return pos_; }
        vec3 Dir() { return currDir_; }
        vec3 Up() { return currUp_; }
        vec3 Right() { return currRight_; }

        // Setters
        void VelX(float x) { vel_.x = x; }
        void VelY(float y) { vel_.y = y; }
        void VelZ(float z) { vel_.z = z; }
        void Vel(vec3 vel) { vel_ = vel; }
        void Pos(vec3 p) { pos_ = p; }

    protected:
        vec3 pos_;
        vec3 initialDir_;
        vec3 initialUp_;
        vec3 initialRight_;
        vec3 currDir_;
        vec3 currUp_;
        vec3 currRight_;
        vec3 vel_;

        float speedAngular_;
        float speedLinear_;
        vec2 rotation_;

        mat4 view_;
        mat4 projection_;
};

#endif  // SRC_INCLUDE_CAMERA_H_
