#include "graphics/render_system.hpp"
#include "core/assert.hpp"
#include "core/scene.hpp"
#include "core/time.hpp"
#include "core/window.hpp"
#include "graphics/graphics_api.hpp"
#include "graphics/pg_to_vulkan_types.hpp"
#include "resource/resource_manager.hpp"
#include "resource/image.hpp"
#include "resource/model.hpp"
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
            s_samplers[name] = Gfx::g_renderState.device.NewSampler( desc );
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
static DescriptorPool s_descriptorPool;
VkDescriptorSetLayout descriptorSetLayout;
std::vector< Progression::Gfx::Buffer > ubos;
VkDescriptorPool descriptorPool;
std::vector<VkDescriptorSet> descriptorSets;
static std::shared_ptr< Image > s_image;

namespace Progression
{

extern bool g_converterMode;

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

        if ( g_converterMode )
        {
            return true;
        }

        ResourceManager::LoadFastFile( PG_RESOURCE_DIR "cache/fastfiles/resource.txt.ff" );
        auto simpleVert = ResourceManager::Get< Shader >( "simpleVert" );
        auto simpleFrag = ResourceManager::Get< Shader >( "simpleFrag" );

        s_image = ResourceManager::Get< Image >( "cockatoo" );

        VertexBindingDescriptor bindingDesc[3];
        bindingDesc[0].binding   = 0;
        bindingDesc[0].stride    = sizeof( glm::vec3 );
        bindingDesc[0].inputRate = VertexInputRate::PER_VERTEX;
        bindingDesc[1].binding   = 1;
        bindingDesc[1].stride    = sizeof( glm::vec3 );
        bindingDesc[1].inputRate = VertexInputRate::PER_VERTEX;
        bindingDesc[2].binding   = 2;
        bindingDesc[2].stride    = sizeof( glm::vec2 );
        bindingDesc[2].inputRate = VertexInputRate::PER_VERTEX;

        std::array< VertexAttributeDescriptor, 3 > attribDescs;
        attribDescs[0].binding  = 0;
        attribDescs[0].location = 0;
        attribDescs[0].format   = BufferDataType::FLOAT3;
        attribDescs[0].offset   = 0;
        attribDescs[1].binding  = 1;
        attribDescs[1].location = 1;
        attribDescs[1].format   = BufferDataType::FLOAT3;
        attribDescs[1].offset   = 0;
        attribDescs[2].binding  = 2;
        attribDescs[2].location = 2;
        attribDescs[2].format   = BufferDataType::FLOAT2;
        attribDescs[2].offset   = 0;
        
        uint32_t numImages = static_cast< uint32_t >( g_renderState.swapChain.images.size() );
        ubos.resize( numImages );
        for ( auto& ubo : ubos )
        {
            ubo = g_renderState.device.NewBuffer( sizeof( glm::mat4 ), BUFFER_TYPE_UNIFORM,
                                                  MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT );
        }

        VkDescriptorPoolSize poolSize[2] = {};
        poolSize[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize[0].descriptorCount = numImages;
        poolSize[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize[1].descriptorCount = numImages;

        s_descriptorPool = g_renderState.device.NewDescriptorPool( 2, poolSize, numImages );

        std::vector< DescriptorSetLayoutData > descriptorSetData = simpleVert->reflectInfo.descriptorSetLayouts;
        descriptorSetData.insert( descriptorSetData.end(), simpleFrag->reflectInfo.descriptorSetLayouts.begin(), simpleFrag->reflectInfo.descriptorSetLayouts.end() );
        auto combined = CombineDescriptorSetLayouts( descriptorSetData );
        PG_ASSERT( combined.size() == 1 );
        const auto& layoutInfo = combined[0].createInfo;
        VkResult ret = vkCreateDescriptorSetLayout( g_renderState.device.GetHandle(), &layoutInfo, nullptr, &descriptorSetLayout );
        PG_ASSERT( ret == VK_SUCCESS );

        std::vector< VkDescriptorSetLayout > layouts( numImages, descriptorSetLayout );
        auto descriptorSets = s_descriptorPool.NewDescriptorSets( numImages, layouts.data() );

        for ( size_t i = 0; i < numImages; i++ )
        {
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = ubos[i].GetHandle();
            bufferInfo.offset = 0;
            bufferInfo.range  = sizeof( glm::mat4 );

            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView   = s_image->GetTexture()->GetView();
            imageInfo.sampler     = GetSampler( "nearest_clamped" )->GetHandle();

            VkWriteDescriptorSet descriptorWrite[2] = {};
            descriptorWrite[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite[0].dstSet           = descriptorSets[i].GetHandle();
            descriptorWrite[0].dstBinding       = 0;
            descriptorWrite[0].dstArrayElement  = 0;
            descriptorWrite[0].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite[0].descriptorCount  = 1;
            descriptorWrite[0].pBufferInfo      = &bufferInfo;

            descriptorWrite[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite[1].dstSet           = descriptorSets[i].GetHandle();
            descriptorWrite[1].dstBinding       = 1;
            descriptorWrite[1].dstArrayElement  = 0;
            descriptorWrite[1].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrite[1].descriptorCount  = 1;
            descriptorWrite[1].pImageInfo       = &imageInfo;

            vkUpdateDescriptorSets( g_renderState.device.GetHandle(), 2, descriptorWrite, 0, nullptr );
        }

        PipelineDescriptor pipelineDesc;
        pipelineDesc.renderPass             = &g_renderState.renderPass;
        pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 3, bindingDesc, 3, attribDescs.data() );
        pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;

        pipelineDesc.viewport = FullScreenViewport();
        pipelineDesc.scissor  = FullScreenScissor();

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

        auto model = ResourceManager::Get< Model >( "cube" );

        for ( size_t i = 0; i < g_renderState.commandBuffers.size(); ++i )
        {
            CommandBuffer& cmdBuf = g_renderState.commandBuffers[i];
            cmdBuf.BeginRecording();
            cmdBuf.BeginRenderPass( g_renderState.renderPass, g_renderState.swapChainFramebuffers[i] );
            cmdBuf.BindRenderPipeline( s_pipeline );
            cmdBuf.BindDescriptorSets( 1, &descriptorSets[i], s_pipeline );

            cmdBuf.BindVertexBuffer( model->meshes[0].vertexBuffer, 0, 0 );
            cmdBuf.BindVertexBuffer( model->meshes[0].vertexBuffer, model->meshes[0].GetNormalOffset(), 1 );
            cmdBuf.BindVertexBuffer( model->meshes[0].vertexBuffer, model->meshes[0].GetUVOffset(), 2 );
            cmdBuf.BindIndexBuffer(  model->meshes[0].indexBuffer,  model->meshes[0].GetIndexType() );
            cmdBuf.DrawIndexed( 0, model->meshes[0].GetNumIndices() );

            cmdBuf.EndRenderPass();
            cmdBuf.EndRecording();
        }

        return true;
    }

    void Shutdown()
    {
        vkDeviceWaitIdle( g_renderState.device.GetHandle() );
        for ( auto& [name, sampler] : s_samplers )
        {
            sampler.Free();
        }
        g_renderState.transientCommandPool.Free();

        ResourceManager::FreeGPUResources();

        if ( !g_converterMode )
        {
            s_descriptorPool.Free();
            vkDestroyDescriptorSetLayout( g_renderState.device.GetHandle(), descriptorSetLayout, nullptr );
            for ( auto& ubo : ubos )
            {
                ubo.Free();
            }
            s_pipeline.Free();
        }

        VulkanShutdown();
    }

    void Render( Scene* scene )
    {  
        size_t currentFrame = g_renderState.currentFrame;
        VkDevice dev = g_renderState.device.GetHandle();
        vkWaitForFences( dev, 1, &g_renderState.inFlightFences[currentFrame], VK_TRUE, UINT64_MAX );
        vkResetFences( dev, 1, &g_renderState.inFlightFences[currentFrame] );

        auto imageIndex = g_renderState.swapChain.AcquireNextImage( g_renderState.presentCompleteSemaphores[currentFrame] );

        glm::mat4 M( 1 );
        // M = glm::translate( M, glm::vec3( 0.0f, -0.7f, 0.0f ) );
        // M = glm::rotate( M, Time::Time() * glm::radians( 90.0f ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
        // M = glm::rotate( M, glm::radians( -90.0f ), glm::vec3( 1.0f, 0.0f, 0.0f ) );
        // M = glm::scale( M, glm::vec3( 1.0 ) );
        M = glm::rotate( M, 0.5f * Time::Time() * glm::radians( 90.0f ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
        M = glm::scale( M, glm::vec3( 0.5 ) );
        auto N = glm::transpose( glm::inverse( M ) );
        auto MVP        = scene->camera.GetVP() * M;
        void* data      = ubos[imageIndex].Map();
        // memcpy( data, &MVP, sizeof( glm::mat4 ) );
        memcpy( data, &M, sizeof( glm::mat4 ) );
        memcpy( (char*)data + sizeof( glm::mat4 ), &N, sizeof( glm::mat4 ) );
        memcpy( (char*)data + 2*sizeof(glm::mat4), &MVP, sizeof( glm::mat4 ) );
        ubos[imageIndex].UnMap();

        g_renderState.device.SubmitRenderCommands( 1, &g_renderState.commandBuffers[imageIndex] );

        g_renderState.device.SubmitFrame( imageIndex );
    }

    void InitSamplers()
    {
        SamplerDescriptor samplerDesc;

        samplerDesc.minFilter = FilterMode::NEAREST;
        samplerDesc.magFilter = FilterMode::NEAREST;
        samplerDesc.wrapModeU = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeV = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeW = WrapMode::CLAMP_TO_EDGE;
        AddSampler( "nearest_clamped", samplerDesc );

        samplerDesc.minFilter = FilterMode::LINEAR;
        samplerDesc.magFilter = FilterMode::LINEAR;
        samplerDesc.wrapModeU = WrapMode::REPEAT;
        samplerDesc.wrapModeV = WrapMode::REPEAT;
        samplerDesc.wrapModeW = WrapMode::REPEAT;
        AddSampler( "linear_clamped", samplerDesc );
    }


} // namespace RenderSystem
} // namespace Progression
