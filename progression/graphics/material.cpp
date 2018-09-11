#include "graphics/material.h"

namespace Progression {

    Material::Material(const glm::vec3& a, const glm::vec3& d, const glm::vec3& s, float ns, Texture* diffuseTex, Shader* sh) :
        ambient(a),
        diffuse(d),
        specular(s),
        shininess(ns),
        diffuseTexture(diffuseTex),
        shader(sh)
    {
    }

} // namespace Progression