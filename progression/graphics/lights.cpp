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

    Light::Light(Light&& light) {
        *this = std::move(light);
    }

    Light& Light::operator=(Light&& light) {
        type            = std::move(light.type);
        color           = std::move(light.color);
        intensity       = std::move(light.intensity);
        radius          = std::move(light.radius);
        innerCutoff     = std::move(light.innerCutoff);
        outerCutoff     = std::move(light.outerCutoff);
        shadowMap       = std::move(light.shadowMap);
        light.shadowMap = nullptr;

        return *this;
    }

} // namespace Progression
