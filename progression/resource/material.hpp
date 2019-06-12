#pragma once

#include "core/common.hpp"
#include "resource/texture2D.hpp"

namespace Progression {

    class Material {
    public:
		Material(
			const glm::vec3& a = glm::vec3(0),
			const glm::vec3& d = glm::vec3(0),
			const glm::vec3& s = glm::vec3(0),
			const glm::vec3& e = glm::vec3(0),
			float ns = 0,
			Texture2D* diffuseTex = nullptr);

        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
		glm::vec3 emissive;
        float shininess;
        Texture2D* diffuseTexture;
    };

} // namespace Progression
