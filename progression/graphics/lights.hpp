#pragma once

#include "core/math.hpp"
#include "graphics/shadow_map.hpp"
#include <memory>

namespace Progression
{

struct Light
{
    glm::vec4 colorAndIntensity = glm::vec4( 1 ); // x,y,z = color, w = intensity
    std::shared_ptr< ShadowMap > shadowMap;
};

struct DirectionalLight : public Light
{
    glm::vec4 direction = glm::vec4( 0, 0, -1, 0 ); // x,y,z = direction, w = padding
};

struct PointLight : public Light
{
    glm::vec4 positionAndRadius = glm::vec4( 0, 0, 0, 10 ); // x,y,z = position, w = radius
};

struct SpotLight : public Light
{
    glm::vec4 positionAndRadius  = glm::vec4( 0, 0, 0, 10 ); // x,y,z = position, w = radius
    glm::vec4 directionAndCutoff = glm::vec4( 0, 0, -1, glm::radians( 20.0f ) ); // x,y,z = direction, w = cutoff
};

} // namespace Progression
