#pragma once

#include "core/common.hpp"
#include "graphics/texture.hpp"

namespace Progression {

    class Shader;
    class Material {
    public:
		Material(
			const glm::vec3& a = glm::vec3(0),
			const glm::vec3& d = glm::vec3(0),
			const glm::vec3& s = glm::vec3(0),
			const glm::vec3& e = glm::vec3(0),
			float ns = 0,
			Texture* diffuseTex = nullptr);

        ~Material() = default;
        Material(const Material& material) = default;
        Material& operator=(const Material& material) = default;
        
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
		glm::vec3 emissive;
        float shininess;
        Texture* diffuseTexture;
    };

} // namespace Progression