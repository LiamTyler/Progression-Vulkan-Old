#include "types/lights.h"

namespace Progression {

    Light::Light(Type _type, const Transform& _transform, const glm::vec3& _color, float _intensity) :
        GameObject(_transform),
        type(_type),
        color(_color),
        intensity(_intensity)
    {
    }

} // namespace Progression