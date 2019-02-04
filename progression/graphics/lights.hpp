#pragma once

#include "core/game_object.hpp"

namespace Progression {

    class Light : public GameObject {
    public:
        enum class Type {
            POINT,
            DIRECTIONAL,
            SPOT
        };

        Light(Type _type = Type::POINT, const Transform& _transform = Transform(),
            const glm::vec3& _color = glm::vec3(1), float _intensity = 2.0f, float _radius = 15.0f, float _innerCutoff = glm::radians(25.0f),
            float _outterCutoff = glm::radians(35.0f)) :
            GameObject(_transform),
            type(_type),
            color(_color),
            intensity(_intensity),
            radius(_radius),
            innerCutoff(_innerCutoff),
            outterCutoff(_outterCutoff)
        {
        }

        virtual ~Light() = default;

        Type type;
        glm::vec3 color;
        float intensity;
        float radius;
        float innerCutoff;
        float outterCutoff;
    };

} // namespace Progression