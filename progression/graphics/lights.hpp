#pragma once

#include "core/common.hpp"

namespace Progression {

    class Light {
    public:
        Light() = default;
        virtual ~Light() = default;

        glm::vec3 color;
        float intensity;
    };

    class PointLight : public Light {
    public:
        PointLight() = default;
        ~PointLight() = default;

        glm::vec3 position;
        float radius;
    };

    class DirectionalLight : public Light {
    public:
        DirectionalLight() = default;
        ~DirectionalLight() = default;

        glm::vec3 direction;
    };

    class SpotLight : public Light {
    public:
        SpotLight() = default;
        ~SpotLight() = default;

        glm::vec3 position;
        float innerCutoff;
        float outerCutoff;
    };

} // namespace Progression
