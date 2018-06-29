#include "include/material.h"

namespace Progression {

	Material::Material(glm::vec3 a, glm::vec3 d, glm::vec3 s, float spec, Texture* tex) :
		ka(a),
		kd(d),
		ks(s),
		specular(spec),
		diffuseTex(tex)
	{
	}

	// NOTE: Should delete diffuse Tex be here?
	Material::~Material() {
	}

} // namespace Progression