#pragma once

#include "core/common.hpp"

namespace Progression {

    class Texture2D;

    class Material {
    public:
		Material(
			const glm::vec3& a = glm::vec3(0),
			const glm::vec3& d = glm::vec3(0),
			const glm::vec3& s = glm::vec3(0),
			const glm::vec3& e = glm::vec3(0),
			float ns = 0,
			Texture2D* diffuseTex = nullptr);

        glm::vec3 Ka;
        glm::vec3 Kd;
        glm::vec3 Ks;
		glm::vec3 Ke;
        float Ns;
        Texture2D* map_Kd;
    };

} // namespace Progression
