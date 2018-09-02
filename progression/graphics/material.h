#pragma once

#include "core/common.h"

namespace Progression {

    class Material {
    public:
        Material();
        Material(const glm::vec3& a, const glm::vec3& d, const glm::vec3& s, float ns);

        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        float shininess;
    };

} // namespace Progression