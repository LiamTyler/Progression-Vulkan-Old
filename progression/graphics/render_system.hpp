#pragma once

#include "graphics/graphics_api.hpp"
#include <string>
#include <vector>

namespace Progression
{

class Scene;

namespace Gfx
{
    class SamplerDescriptor;
    class Sampler;

} // namespace Gfx

namespace RenderSystem
{

    // placed here for sharing between the animation system and render system
    struct GBuffer
    {
        Gfx::Texture positions;
        Gfx::Texture normals;
        Gfx::Texture diffuseAndSpecular;
        Gfx::Texture specular;
    };

    struct GBufferPassData
    {
        Gfx::RenderPass renderPass;
        GBuffer gbuffer;
        Gfx::Framebuffer frameBuffer;
        Gfx::Pipeline pipeline;
        std::vector< Gfx::DescriptorSetLayout > descriptorSetLayouts;
    };

    bool Init();

    void Shutdown();

    void Render( Scene* scene );
    
    void InitSamplers();
    void FreeSamplers();
    Gfx::Sampler* AddSampler( Gfx::SamplerDescriptor& desc );
    Gfx::Sampler* GetSampler( const std::string& name );

} // namespace RenderSystem
} // namespace Progression
