#pragma once

#include "core/common.hpp"
#include "resource/resource.hpp"

namespace Progression {

    class Texture2D;

    class Material : public Resource {
    public:
        Material() = default;

        glm::vec3 Ka;
        glm::vec3 Kd;
        glm::vec3 Ks;
		glm::vec3 Ke;
        float Ns;
        Texture2D* map_Kd;
    };

} // namespace Progression
