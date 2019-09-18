#include "graphics/render_system.hpp"
#include "core/scene.hpp"
#include "core/window.hpp"
#include "graphics/graphics_api.hpp"
// #include "resource/shader.hpp"
#include "utils/logger.hpp"
//#include "graphics/shadow_map.hpp"
#include <array>
// #include "graphics/pg_to_opengl_types.hpp"

using namespace Progression;
using namespace Gfx;

static std::unordered_map< std::string, Gfx::Sampler > s_samplers;

namespace Progression
{
namespace RenderSystem
{

    Gfx::Sampler* AddSampler( const std::string& name, Gfx::SamplerDescriptor& desc )
    {
        auto it = s_samplers.find( name );
        if ( it != s_samplers.end() )
        {
            return &it->second;
        }
        else
        {
            s_samplers[name] = Gfx::Sampler::Create( desc );
            return &s_samplers[name];
        }
    }

    Gfx::Sampler* GetSampler( const std::string& name )
    {
        auto it = s_samplers.find( name );
        if ( it != s_samplers.end() )
        {
            return &it->second;
        }

        return nullptr;
    }

} // namespace RenderSystem
} // namespace Progression

static Window* s_window;

namespace Progression
{
namespace RenderSystem
{

    void InitSamplers();

    bool Init()
    {
        InitSamplers();
        s_window = GetMainWindow();

        return true;
    }

    void Shutdown()
    {
        for ( auto& [name, sampler] : s_samplers )
        {
            sampler = {};
        }
    }

    void Render( Scene* scene )
    {
        PG_UNUSED( scene );
    }

    void InitSamplers()
    {
        SamplerDescriptor samplerDesc;

        samplerDesc.minFilter = FilterMode::NEAREST;
        samplerDesc.magFilter = FilterMode::NEAREST;
        samplerDesc.wrapModeS = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeT = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeR = WrapMode::CLAMP_TO_EDGE;
        //s_samplers["nearest"] = Sampler::Create( samplerDesc );

        samplerDesc.minFilter = FilterMode::LINEAR;
        samplerDesc.magFilter = FilterMode::LINEAR;
        //s_samplers["linear"] = Sampler::Create( samplerDesc );
    }


} // namespace RenderSystem
} // namespace Progression
