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

    /**
     * \brief Initialze the main rendering engine. This includes the shaders,
     *        GBuffer, etc.
     */
    bool Init();

    /**
     * \brief Free all of the resources used by the rendering engine and shutdown.
     */
    void Shutdown();

    /**
     * \brief Will render the given scene.
     */
    void Render( Scene* scene );

    
    Gfx::Sampler* AddSampler( const std::string& name, Gfx::SamplerDescriptor& desc );
    Gfx::Sampler* GetSampler( const std::string& name );

} // namespace RenderSystem
} // namespace Progression
