#include "graphics/render_system.hpp"
#include "core/assert.hpp"
#include "core/scene.hpp"
#include "core/window.hpp"
#include "graphics/graphics_api.hpp"
#include "resource/resource_manager.hpp"
#include "resource/shader.hpp"
#include "utils/logger.hpp"
#include <array>
#include <unordered_map>
#include "graphics/vulkan.hpp"

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
static Pipeline s_pipeline;
static VkRenderPass renderPass;
static VkPipeline graphicsPipeline;

namespace Progression
{
namespace RenderSystem
{

    void InitSamplers();

    bool Init()
    {
        if ( !VulkanInit() )
        {
            LOG_ERR( "Could not initialize vulkan" );
            return false;
        }

        InitSamplers();
        s_window = GetMainWindow();

        PipelineDescriptor pipelineDesc;
        pipelineDesc.renderPass       = &g_renderState.renderPass;
        pipelineDesc.vertexDescriptor = VertexInputDescriptor::Create( 0, nullptr, 0, nullptr );
        pipelineDesc.rasterizerInfo.winding = WindingOrder::CLOCKWISE;

        pipelineDesc.viewport.x        = 0.0f;
        pipelineDesc.viewport.y        = 0.0f;
        pipelineDesc.viewport.width    = static_cast< float >( g_renderState.swapChain.extent.width );
        pipelineDesc.viewport.height   = static_cast< float >( g_renderState.swapChain.extent.height );
        pipelineDesc.viewport.minDepth = 0.0f;
        pipelineDesc.viewport.maxDepth = 1.0f;

        pipelineDesc.scissor.x      = 0;
        pipelineDesc.scissor.y      = 0;
        pipelineDesc.scissor.width  = g_renderState.swapChain.extent.width;
        pipelineDesc.scissor.height = g_renderState.swapChain.extent.height;

        ResourceManager::LoadFastFile( PG_RESOURCE_DIR "cache/fastfiles/resource.txt.ff" );
        auto simpleVert = ResourceManager::Get< Shader >( "simpleVert" );
        auto simpleFrag = ResourceManager::Get< Shader >( "simpleFrag" );
        pipelineDesc.shaders[0] = simpleVert.get();
        pipelineDesc.shaders[1] = simpleFrag.get();
        pipelineDesc.numShaders = 2;

        s_pipeline = Pipeline::Create( pipelineDesc );
        if ( !s_pipeline )
        {
            LOG_ERR( "Could not create pipeline" );
            return false;
        }

        simpleVert->Free();
        simpleFrag->Free();

        for ( size_t i = 0; i < g_renderState.commandBuffers.size(); ++i )
        {
            CommandBuffer& cmdBuf = g_renderState.commandBuffers[i];
            cmdBuf.BeginRecording();
            cmdBuf.BeginRenderPass( g_renderState.renderPass, g_renderState.swapChainFramebuffers[i] );
            cmdBuf.BindRenderPipeline( s_pipeline );
            cmdBuf.Draw( 0, 3 );
            cmdBuf.EndRenderPass();
            cmdBuf.EndRecording();
        }

        return true;
    }

    void Shutdown()
    {
        vkDeviceWaitIdle( g_renderState.device.GetNativeHandle() );
        for ( auto& [name, sampler] : s_samplers )
        {
            sampler.Free();
        }

        s_pipeline.Free();

        VulkanShutdown();
    }

    void Render( Scene* scene )
    {
        PG_UNUSED( scene );
        
        size_t currentFrame = g_renderState.currentFrame;
        VkDevice dev = g_renderState.device.GetNativeHandle();
        vkWaitForFences( dev, 1, &g_renderState.inFlightFences[currentFrame], VK_TRUE, UINT64_MAX );
        vkResetFences( dev, 1, &g_renderState.inFlightFences[currentFrame] );

        auto imageIndex = g_renderState.swapChain.AcquireNextImage( g_renderState.presentCompleteSemaphores[currentFrame] );
        g_renderState.device.SubmitRenderCommands( 1, &g_renderState.commandBuffers[imageIndex] );

        g_renderState.device.SubmitFrame( imageIndex );
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
