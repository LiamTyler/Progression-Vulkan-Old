#pragma once

#include "core/math.hpp"

namespace Progression
{

class Transform
{
public:
    Transform() = default;

    glm::mat4 GetModelMatrix() const;

    glm::vec3 position = glm::vec3( 0 );
    glm::vec3 rotation = glm::vec3( 0 );
    glm::vec3 scale    = glm::vec3( 1 );
};

} // namespace Progression
