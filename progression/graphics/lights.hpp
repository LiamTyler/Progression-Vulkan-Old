#pragma once

#include "core/game_object.hpp"

namespace Progression {

    class ShadowMap;

    class Light : public GameObject {
    public:
        enum Type {
            POINT = 0,
            SPOT,
            DIRECTIONAL,
        };

        Light(
            Type _type = Type::POINT,
            const Transform& _transform = Transform(),
            const glm::vec3& _color = glm::vec3(1),
            float _intensity = 2.0f,
            float _radius = 15.0f,
            float _innerCutoff = glm::radians(25.0f),
            float _outerCutoff = glm::radians(35.0f)
        );

        virtual ~Light();
        Light(Light&& light);
        Light& operator=(Light&& light);

        Type type;
        glm::vec3 color;
        float intensity;
        float radius;
        float innerCutoff;
        float outerCutoff;
        ShadowMap* shadowMap;
    };

} // namespace Progression
