#pragma once

#include "core/common.h"
#include "types/texture.h"

namespace Progression {

    class Shader;
    class Material {
    public:
        Material(
            const glm::vec3& a = glm::vec3(0),
            const glm::vec3& d = glm::vec3(0),
            const glm::vec3& s = glm::vec3(0),
            float ns = 0,
            Texture* diffuseTex = nullptr,
            Shader* shader = nullptr);

        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        float shininess;
        Texture* diffuseTexture;
        Shader* shader;
    };

} // namespace Progression