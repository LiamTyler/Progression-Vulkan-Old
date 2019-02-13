#pragma once

#include "core/game_object.hpp"
#include "core/frustum.hpp"

namespace Progression {

	class Camera : public GameObject {
		public:
            struct RenderOptions {
                float exposure = 1.0f;
            };

			Camera(
				const Transform& t = Transform(),
				float fovy = glm::radians(45.0f),
				float a = 16.0f / 9.0f,
				float np = .1f,
				float fp = 100.0f);

            void UpdateFrustum(const glm::vec3& position, const glm::vec3& rotation);
			void UpdateOrientationVectors();
			void UpdateViewMatrix();

			float GetFOV() const;
			float GetAspectRatio() const;
			float GetNearPlane() const;
			float GetFarPlane() const;
			glm::mat4 GetV() const;
			glm::mat4 GetP() const;
			glm::vec3 GetForwardDir() const;
			glm::vec3 GetUpDir() const;
			glm::vec3 GetRightDir() const;
			Frustum GetFrustum() const;

			void SetFOV(float f);
			void SetAspectRatio(float a);
			void SetNearPlane(float p);
			void SetFarPlane(float p);

            struct RenderOptions renderOptions;

		protected:
			void UpdateProjectionMatrix();
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
