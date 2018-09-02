#include "graphics/material.h"

namespace Progression {

    Material::Material() :
        ambient(0),
        diffuse(0),
        specular(0),
        shininess(0)
    {
    }

    Material::Material(const glm::vec3& a, const glm::vec3& d, const glm::vec3& s, float ns) :
        ambient(a),
        diffuse(d),
        specular(s),
        shininess(ns)
    {
    }

} // namespace Progression