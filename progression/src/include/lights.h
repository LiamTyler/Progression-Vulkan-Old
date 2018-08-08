#pragma once

#include "include/utils.h"

namespace Progression {

	class Light {
	public:
		Light(
			const glm::vec3& a = glm::vec3(1.0f),
			const glm::vec3& d = glm::vec3(1.0f),
			const glm::vec3& s = glm::vec3(1.0f));

		glm::vec3 Ia;
		glm::vec3 Id;
		glm::vec3 Is;
	};

	class DirectionalLight : public Light {
	public:
		DirectionalLight(const glm::vec3& dir = glm::vec3(0, 0, -1));
		DirectionalLight(const glm::vec3& dir, const glm::vec3& a, const glm::vec3& d, const glm::vec3& s);

		glm::vec3 direction;
	};

	class PointLight : public Light {
	public:
		PointLight(const glm::vec3& pos = glm::vec3(0));
		PointLight(const glm::vec3& pos, const glm::vec3& a, const glm::vec3& d, const glm::vec3& s);

		glm::vec3 position;
	};

} // namespace Progression
