#pragma once

#include "core/common.hpp"
#include "graphics/texture2D.hpp"

namespace Progression {

    class Material {
    public:
		Material(
			const glm::vec3& a = glm::vec3(0),
			const glm::vec3& d = glm::vec3(0),
			const glm::vec3& s = glm::vec3(0),
			const glm::vec3& e = glm::vec3(0),
			float ns = 0,
			std::shared_ptr<Texture2D> diffuseTex = nullptr);

        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
		glm::vec3 emissive;
        float shininess;
        std::shared_ptr<Texture2D> diffuseTexture;
    };

} // namespace Progression
