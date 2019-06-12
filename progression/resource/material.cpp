#include "resource/material.hpp"

namespace Progression {

    Material::Material(const glm::vec3& a, const glm::vec3& d, const glm::vec3& s,
					   const glm::vec3& e, float ns, Texture2D* diffuseTex) :
        ambient(a),
        diffuse(d),
		specular(s),
		emissive(e),
		shininess(ns),
        diffuseTexture(diffuseTex)
    {
    }

} // namespace Progression
