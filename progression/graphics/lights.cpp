#include "graphics/lights.hpp"
#include "graphics/shadow_map.hpp"

namespace Progression {

    Light::Light(Type _type, const Transform& _transform, const glm::vec3& _color,
        float _intensity, float _radius, float _innerCutoff, float _outerCutoff) :
        GameObject(_transform),
        type(_type),
        color(_color),
        intensity(_intensity),
        radius(_radius),
        innerCutoff(_innerCutoff),
        outerCutoff(_outerCutoff),
        shadowMap(nullptr)
    {
    }

    Light::~Light() {
        if (shadowMap)
            delete shadowMap;
    }

} // namespace Progression
