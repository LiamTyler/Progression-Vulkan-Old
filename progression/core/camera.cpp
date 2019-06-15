#include "core/camera.hpp"

namespace Progression {

	Camera::Camera(const glm::vec3& pos, const glm::vec3& rot, float fovy, float a, float np, float fp) :
        position(pos),
        rotation(rot),
		fieldOfView_(fovy),
		aspectRatio_(a),
		nearPlane_(np),
		farPlane_(fp)
	{
        updateOrientationVectors();
        updateViewMatrix();
		updateProjectionMatrix();
	}

    void Camera::updateFrustum(const glm::vec3& pos, const glm::vec3& rot) {
        position = pos;
        rotation = rot;
		updateOrientationVectors();
		updateViewMatrix();
        frustum_.Update(fieldOfView_, nearPlane_, farPlane_, aspectRatio_, position,
                currDir_, currUp_, currRight_);
    }

	void Camera::updateOrientationVectors() {
		glm::mat4 rot(1);
        // TODO: glm can do rotation around an axis already
		rot = glm::rotate(rot, rotation.y, glm::vec3(0, 1, 0));
		rot = glm::rotate(rot, rotation.x, glm::vec3(1, 0, 0));
		currDir_ = glm::vec3(rot * glm::vec4(0, 0, -1, 0));
		currUp_ = glm::vec3(rot * glm::vec4(0, 1, 0, 0));
		currRight_ = glm::cross(currDir_, currUp_);
	}

	void Camera::updateViewMatrix() {
		viewMatrix_ = glm::lookAt(
			position,
			position + currDir_,
			currUp_);
	}

	void Camera::updateProjectionMatrix() {
		projectionMatrix_ = glm::perspective(fieldOfView_, aspectRatio_, nearPlane_, farPlane_);
	}

	glm::mat4 Camera::getV() const { return viewMatrix_; }
	glm::mat4 Camera::getP() const { return projectionMatrix_; }
	glm::mat4 Camera::getVP() const { return projectionMatrix_ * viewMatrix_; }
	float Camera::getFOV() const { return fieldOfView_; }
	float Camera::getAspectRatio() const { return aspectRatio_; }
	float Camera::getNearPlane() const { return nearPlane_; }
	float Camera::getFarPlane() const { return farPlane_; }
	glm::vec3 Camera::getForwardDir() const { return currDir_; }
	glm::vec3 Camera::getUpDir() const { return currUp_; }
	glm::vec3 Camera::getRightDir() const { return currRight_; }
	Frustum Camera::getFrustum() const { return frustum_; }

	void Camera::setFOV(float f) {
		fieldOfView_ = f;
		updateProjectionMatrix();
	}

	void Camera::setAspectRatio(float a) {
		aspectRatio_ = a;
		updateProjectionMatrix();
	}

	void Camera::setNearPlane(float p) {
		nearPlane_ = p;
		updateProjectionMatrix();
	}

	void Camera::setFarPlane(float p) {
		farPlane_ = p;
		updateProjectionMatrix();
	}

} // namespace Progression
