#include "core/camera.hpp"

namespace Progression {

	Camera::Camera(const Transform& t, float fovy, float a, float np, float fp) :
        GameObject(t),
		fieldOfView_(fovy),
		aspectRatio_(a),
		nearPlane_(np),
		farPlane_(fp),
		renderingPipeline_(RenderingPipeline::FORWARD)
	{
		UpdateProjectionMatrix();
	}

    void Camera::UpdateFrustum(const glm::vec3& position, const glm::vec3& rotation) {
        transform.position = position;
        transform.rotation = rotation;
		UpdateOrientationVectors();
		UpdateViewMatrix();
        frustum_.Update(fieldOfView_, nearPlane_, farPlane_, aspectRatio_, transform.position,
                currDir_, currUp_, currRight_);
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
	RenderingPipeline Camera::GetRenderingPipeline() const { return renderingPipeline_; }
	Frustum Camera::GetFrustum() const { return frustum_; }

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

	void Camera::SetRenderingPipeline(RenderingPipeline p) {
		renderingPipeline_ = p;
	}

} // namespace Progression
