#include "graphics/shadow_map.hpp"
#include "graphics/vulkan.hpp"
#include "graphics/render_system.hpp"

extern Progression::RenderSystem::ShadowPassData shadowPassData;

namespace Progression
{

using namespace Gfx;

bool ShadowMap::Init()
{
    ImageDescriptor desc = {};
    desc.width    = width;
    desc.height   = height;
    desc.type     = ImageType::TYPE_2D;
    desc.format   = PixelFormat::DEPTH_32_FLOAT;
    desc.sampler  = "shadow_map";
    desc.usage    = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    texture       = g_renderState.device.NewTexture( desc );
    if ( !texture )
    {
        return false;
    }
    framebuffer   = g_renderState.device.NewFramebuffer( { &texture }, shadowPassData.renderPass );
    if ( !framebuffer )
    {
        return false;
    }
    
    return true;
}

void ShadowMap::Free()
{
    framebuffer.Free();
    texture.Free();
}

} // namespace Progression