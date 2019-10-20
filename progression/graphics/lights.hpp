#pragma once

#include "glm/vec4.hpp"

namespace Progression
{

    struct Light
    {
        glm::vec4 colorAndIntensity = glm::vec4( 1 ); // x, y, z = color, w = intensity
    };

    struct DirectionalLight : public Light
    {
        glm::vec3 direction = glm::vec3( 0, 0, -1 );
    };


    struct PointLight : public Light
    {
        glm::vec3 position = glm::vec3( 0 );
        float radius       = 10.0f;
    };

    struct SpotLight : public Light
    {
        glm::vec3 position  = glm::vec3( 0 );
        float radius        = 10.0f;
        glm::vec3 direction = glm::vec3( 0, 0, -1 );
        float cutoff        = glm::radians( 10.0f );
    };

} // namespace Progression
