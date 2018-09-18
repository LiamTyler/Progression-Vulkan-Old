#pragma once

#include "core/game_object.h"

namespace Progression {

    class Light : public GameObject {
    public:
        enum class Type {
            POINT,
            DIRECTIONAL
        };

        Light(Type type = Type::POINT, const Transform& transform = Transform(),
            const glm::vec3& color = glm::vec3(1), float intensity = 1.0f);
        virtual ~Light() = default;

        Type type;
        glm::vec3 color;
        float intensity;
    };

} // namespace Progression