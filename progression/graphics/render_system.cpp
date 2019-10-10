#include "graphics/render_system.hpp"
#include "core/assert.hpp"
#include "core/scene.hpp"
#include "core/time.hpp"
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
static Buffer s_indexBuffer;
static DescriptorPool s_descriptorPool;
VkDescriptorSetLayout descriptorSetLayout;
std::vector< Progression::Gfx::Buffer > ubos;
VkDescriptorPool descriptorPool;
std::vector<VkDescriptorSet> descriptorSets;

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

        g_renderState.transientCommandPool = g_renderState.device.NewCommandPool( COMMAND_POOL_TRANSIENT );

        InitSamplers();
        s_window = GetMainWindow();

        ResourceManager::LoadFastFile( PG_RESOURCE_DIR "cache/fastfiles/resource.txt.ff" );
        auto simpleVert = ResourceManager::Get< Shader >( "simpleVert" );
        auto simpleFrag = ResourceManager::Get< Shader >( "simpleFrag" );


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
            glm::vec3( -0.5, -0.5, 0 ), glm::vec3( 1, 0, 0 ),
            glm::vec3( -0.5,  0.5, 0 ), glm::vec3( 1, 1, 1 ),
            glm::vec3(  0.5,  0.5, 0 ), glm::vec3( 0, 0, 1 ),
            glm::vec3(  0.5, -0.5, 0 ), glm::vec3( 0, 1, 0 ),
        };

        std::vector< uint16_t > indices = { 0, 1, 2, 2, 3, 0 };

        s_buffer      = g_renderState.device.NewBuffer( sizeof( vertices ), vertices, BUFFER_TYPE_VERTEX, MEMORY_TYPE_DEVICE_LOCAL );
        s_indexBuffer = g_renderState.device.NewBuffer( indices.size() * sizeof( uint16_t ), indices.data(), BUFFER_TYPE_INDEX, MEMORY_TYPE_DEVICE_LOCAL );

        ubos.resize( g_renderState.swapChain.images.size() );
        for ( auto& ubo : ubos )
        {
            ubo = g_renderState.device.NewBuffer( sizeof( glm::mat4 ), BUFFER_TYPE_UNIFORM,
                                                  MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT );
        }


        VkDescriptorPoolSize poolSize = {};
        poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast< uint32_t >( g_renderState.swapChain.images.size() );

        s_descriptorPool = g_renderState.device.NewDescriptorPool( 1, &poolSize, g_renderState.swapChain.images.size() );

        const auto& layoutInfo = simpleVert->reflectInfo.descriptorSetLayouts[0].createInfo;
        VkResult ret = vkCreateDescriptorSetLayout( g_renderState.device.GetNativeHandle(), &layoutInfo, nullptr, &descriptorSetLayout );
        PG_ASSERT( ret == VK_SUCCESS );

        auto numImages = g_renderState.swapChain.images.size();
        std::vector< VkDescriptorSetLayout > layouts( g_renderState.swapChain.images.size(), descriptorSetLayout );
        auto descriptorSets = s_descriptorPool.NewDescriptorSets( numImages, layouts.data() );

        for ( size_t i = 0; i < g_renderState.swapChain.images.size(); i++ )
        {
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = ubos[i].GetNativeHandle();
            bufferInfo.offset = 0;
            bufferInfo.range  = sizeof( glm::mat4 );
            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet           = descriptorSets[i].GetNativeHandle();
            descriptorWrite.dstBinding       = 0;
            descriptorWrite.dstArrayElement  = 0;
            descriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount  = 1;
            descriptorWrite.pBufferInfo      = &bufferInfo;
            descriptorWrite.pImageInfo       = nullptr; // Optional
            descriptorWrite.pTexelBufferView = nullptr; // Optional
            vkUpdateDescriptorSets( g_renderState.device.GetNativeHandle(), 1, &descriptorWrite, 0, nullptr );
        }

        PipelineDescriptor pipelineDesc;
        pipelineDesc.renderPass             = &g_renderState.renderPass;
        pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 1, &bindingDesc, 2, attribDescs.data() );
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
            cmdBuf.BindVertexBuffer( s_buffer );
            cmdBuf.BindIndexBuffer( s_indexBuffer, IndexType::UNSIGNED_SHORT );
            cmdBuf.BindDescriptorSets( 1, &descriptorSets[i], s_pipeline );
            cmdBuf.DrawIndexed( 0, 6 );
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

        s_descriptorPool.Free();
        vkDestroyDescriptorSetLayout( g_renderState.device.GetNativeHandle(), descriptorSetLayout, nullptr );
        for ( auto& ubo : ubos )
        {
            ubo.Free();
        }
        s_buffer.Free();
        s_indexBuffer.Free();
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
   
        glm::mat4 model = glm::rotate( glm::mat4( 1.0f ), Time::Time() * glm::radians( 90.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );
        auto MVP        = scene->camera.GetVP() * model;
        void* data      = ubos[imageIndex].Map();
        memcpy( data, &MVP, sizeof( glm::mat4 ) );
        ubos[imageIndex].UnMap();

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
