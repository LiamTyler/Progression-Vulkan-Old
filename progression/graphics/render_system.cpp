#include "graphics/render_system.hpp"
#include "core/assert.hpp"
#include "core/scene.hpp"
#include "core/window.hpp"
#include "graphics/graphics_api.hpp"
#include "graphics/pg_to_vulkan_types.hpp"
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
            PG_UNUSED( desc );
            //s_samplers[name] = Gfx::Sampler::Create( desc );
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
static Buffer s_buffer;

namespace Progression
{
namespace RenderSystem
{

    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 color;
    };

    void InitSamplers();

    bool Init()
    {
        if ( !VulkanInit() )
        {
            LOG_ERR( "Could not initialize vulkan" );
            return false;
        }

        g_renderState.transientCommandPool = g_renderState.device.NewCommandPool( CommandPoolFlags::TRANSIENT );

        InitSamplers();
        s_window = GetMainWindow();

        VertexBindingDescriptor bindingDesc;
        bindingDesc.binding   = 0;
        bindingDesc.stride    = sizeof( Vertex );
        bindingDesc.inputRate = VertexInputRate::PER_VERTEX;

        std::array< VertexAttributeDescriptor, 2 > attribDescs;
        attribDescs[0].binding  = 0;
        attribDescs[0].location = 0;
        attribDescs[0].format   = BufferDataType::FLOAT3;
        attribDescs[0].offset   = 0;

        attribDescs[1].binding  = 0;
        attribDescs[1].location = 1;
        attribDescs[1].format   = BufferDataType::FLOAT3;
        attribDescs[1].offset   = sizeof( glm::vec3 );

        glm::vec3 vertices[] =
        {
            glm::vec3(    0, -0.5, 0 ), glm::vec3( 1, 0, 0 ),
            glm::vec3( -0.5,  0.5, 0 ), glm::vec3( 0, 0, 1 ),
            glm::vec3(  0.5,  0.5, 0 ), glm::vec3( 0, 1, 0 ),
        };

        Buffer stagingBuffer = g_renderState.device.NewBuffer( sizeof( vertices ), BufferType::TRANSFER_SRC, MemoryType::HOST_VISIBLE | MemoryType::HOST_COHERENT );
        void* data = stagingBuffer.Map();
        memcpy( data, vertices, stagingBuffer.GetLength() );
        stagingBuffer.UnMap();

        s_buffer = g_renderState.device.NewBuffer( sizeof( vertices ), BufferType::TRANSFER_DST | BufferType::VERTEX, MemoryType::DEVICE_LOCAL );
        g_renderState.device.Copy( s_buffer, stagingBuffer );
        stagingBuffer.Free();

        PipelineDescriptor pipelineDesc;
        pipelineDesc.renderPass             = &g_renderState.renderPass;
        pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 1, &bindingDesc, 2, attribDescs.data() );
        pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;

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

        s_pipeline = g_renderState.device.NewPipeline( pipelineDesc );
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
            VkBuffer vertexBuffers[] = { s_buffer.GetNativeHandle() };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers( cmdBuf.GetNativeHandle(), 0, 1, vertexBuffers, offsets );
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

        s_buffer.Free();
        s_pipeline.Free();
        g_renderState.transientCommandPool.Free();

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
