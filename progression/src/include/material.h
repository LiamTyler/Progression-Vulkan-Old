#pragma once

#include "include/utils.h"
#include "include/texture.h"

namespace Progression {

	class Material {
	public:
		Material(
			glm::vec3 a = glm::vec3(0),
			glm::vec3 d = glm::vec3(0),
			glm::vec3 s = glm::vec3(0),
			float spec = 0,
			Texture* tex = nullptr);
		~Material();

		glm::vec3 ka;
		glm::vec3 kd;
		glm::vec3 ks;
		float specular;
		Texture* diffuseTex;
	};

} // namespace Progression