#include "resource/material.hpp"

namespace Progression {

    Material::Material(const glm::vec3& a, const glm::vec3& d, const glm::vec3& s,
					   const glm::vec3& e, float ns, Texture2D* diffuseTex) :
        Ka(a),
        Kd(d),
		Ks(s),
		Ke(e),
		Ns(ns),
        map_Kd(diffuseTex)
    {
    }

} // namespace Progression
