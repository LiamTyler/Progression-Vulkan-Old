#pragma once

#include "core/math.hpp"

namespace Progression
{

    class Light
    {
    public:
        enum Type
        {
            POINT = 0,
            DIRECTIONAL,
            SPOT
        };

        std::string name    = "";
        Type type           = POINT;
        glm::vec3 color     = glm::vec3(1, 1, 1);
        glm::vec3 position  = glm::vec3(0, 0, 0);
        glm::vec3 direction = glm::vec3(0, 0, 0);
        float intensity     = 1.0f;
        float radius        = 10.0f;
        float innerCutoff   = glm::radians(10.0f);
        float outerCutoff   = glm::radians(20.0f);
        bool enabled        = true;
    };

} // namespace Progression
