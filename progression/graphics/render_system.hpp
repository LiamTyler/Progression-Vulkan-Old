#pragma once

#include <string>

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

    bool Init();

    void Shutdown();

    void Render( Scene* scene );
    
    void InitSamplers();
    Gfx::Sampler* AddSampler( Gfx::SamplerDescriptor& desc );
    Gfx::Sampler* GetSampler( const std::string& name );

} // namespace RenderSystem
} // namespace Progression
