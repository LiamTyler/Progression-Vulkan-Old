#pragma once

#include "include/utils.h"
#include "include/transform.h"

class Camera {
    public:
        Camera();
        Camera(const glm::vec3& pos);
        Camera(Transform t);
        Camera(Transform t, float fov, float a, float np, float fp);

        glm::mat4 GetV() const;
        glm::mat4 GetP() const;
        float GetFOV() const;
        float GetAspectRatio() const;
        float GetNearPlane() const;
        float GetFarPlane() const;
        void SetFOV(float f);
        void SetAspectRatio(float a);
        void SetNearPlane(float p);
        void SetFarPlane(float p);
        void UpdateOrientationVectors();
        void UpdateViewMatrix();
        glm::vec3 GetForwardDir() const;
        glm::vec3 GetUpDir() const;
        glm::vec3 GetRightDir() const;

		Transform transform;

    protected:
        void UpdateProjectionMatrix();
        float FOV;
        float aspectRatio;
        float nearPlane;
        float farPlane;
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;
        glm::vec3 currDir;
        glm::vec3 currUp;
        glm::vec3 currRight;
};
