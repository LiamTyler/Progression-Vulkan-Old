#pragma once

#include "core/game_object.hpp"
#include "core/frustum.hpp"

namespace Progression {

	class Camera {
	public:
        struct RenderOptions {
            float exposure = 1.0f;
        };

        Camera(
            const glm::vec3& pos = glm::vec3(0),
            const glm::vec3& rot = glm::vec3(0),
            float fovy = glm::radians(45.0f),
            float a = 16.0f / 9.0f,
            float np = .1f,
            float fp = 100.0f);

        void updateFrustum(const glm::vec3& position, const glm::vec3& rotation);
        void updateOrientationVectors();
        void updateViewMatrix();

        float getFOV() const;
        float getAspectRatio() const;
        float getNearPlane() const;
        float getFarPlane() const;
        glm::mat4 getV() const;
        glm::mat4 getP() const;
        glm::mat4 getVP() const;
        glm::vec3 getForwardDir() const;
        glm::vec3 getUpDir() const;
        glm::vec3 getRightDir() const;
        Frustum   getFrustum() const;

        void setFOV(float f);
        void setAspectRatio(float a);
        void setNearPlane(float p);
        void setFarPlane(float p);

        struct RenderOptions renderOptions;
        glm::vec3 position;
        glm::vec3 rotation;

	protected:
        void updateProjectionMatrix();
        float fieldOfView_;
        float aspectRatio_;
        float nearPlane_;
        float farPlane_;
        glm::mat4 viewMatrix_;
        glm::mat4 projectionMatrix_;
        glm::vec3 currDir_;
        glm::vec3 currUp_;
        glm::vec3 currRight_;
        Frustum frustum_;
	};

} // namespace Progression
