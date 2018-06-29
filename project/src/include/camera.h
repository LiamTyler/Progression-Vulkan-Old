#pragma once

#include "include/utils.h"
#include "include/transform.h"

namespace Progression {

	class Camera {
		public:
			Camera(
				const Transform& t = Transform(),
				float fov = glm::radians(45.0f),
				float a = 4.0f / 3.0f,
				float np = .1f,
				float fp = 100.0f);

			float GetFOV() const;
			float GetAspectRatio() const;
			float GetNearPlane() const;
			float GetFarPlane() const;
			glm::mat4 GetV() const;
			glm::mat4 GetP() const;
			glm::vec3 GetForwardDir() const;
			glm::vec3 GetUpDir() const;
			glm::vec3 GetRightDir() const;

			void SetFOV(float f);
			void SetAspectRatio(float a);
			void SetNearPlane(float p);
			void SetFarPlane(float p);
			void UpdateOrientationVectors();
			void UpdateViewMatrix();


			Transform transform;

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
	};

} // namespace Progression