#pragma once

#include "core/math.hpp"
#include "graphics/graphics_api/texture.hpp"
#include "graphics/graphics_api/framebuffer.hpp"

namespace Progression
{

class ShadowMap
{
public:
    ShadowMap() = default;
    
    bool Init();
    void Free();

    Gfx::Texture texture;
    Gfx::Framebuffer framebuffer;
    glm::mat4 LSM;
    float constantBias = 2;
    float slopeBias    = 9;
    uint32_t width     = 2048;
    uint32_t height    = 2048;
};

} // namespace Progression
