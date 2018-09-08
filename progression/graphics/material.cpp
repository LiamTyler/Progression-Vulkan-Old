#include "graphics/material.h"

namespace Progression {

    Material::Material(const glm::vec3& a, const glm::vec3& d, const glm::vec3& s, float ns, Shader* sh) :
        ambient(a),
        diffuse(d),
        specular(s),
        shininess(ns),
        shader(sh)
    {
    }

} // namespace Progression