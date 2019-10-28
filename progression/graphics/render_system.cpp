#include "graphics/render_system.hpp"
#include "core/assert.hpp"
#include "core/scene.hpp"
#include "core/time.hpp"
#include "core/window.hpp"
#include "components/model_renderer.hpp"
#include "graphics/graphics_api.hpp"
#include "graphics/pg_to_vulkan_types.hpp"
#include "graphics/texture_manager.hpp"
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

struct PerObjectConstantBuffer
{
    glm::mat4 modelMatrix; 
    glm::mat4 normalMatrix;
};

struct SceneConstantBuffer
{
    alignas( 16 ) glm::mat4 VP;
    alignas( 16 ) glm::vec3 cameraPos;
    alignas( 16 ) glm::vec3 ambientColor;
    alignas( 16 ) DirectionalLight dirLight;
    uint32_t numPointLights;
    uint32_t numSpotLights;
};

struct MaterialConstantBuffer
{
    glm::vec4 Ka;
    glm::vec4 Kd;
    glm::vec4 Ks;
    uint32_t diffuseTextureSlot;
};

static Window* s_window;
static Pipeline s_pipeline;
static DescriptorPool s_descriptorPool;
static std::vector< DescriptorSetLayout > s_descriptorSetLayouts;
static std::vector< Progression::Gfx::Buffer > s_gpuSceneConstantBuffers;
static std::vector< Progression::Gfx::Buffer > s_gpuPointLightBuffers;
std::vector< DescriptorSet > sceneDescriptorSets;
std::vector< DescriptorSet > textureDescriptorSets;
static std::shared_ptr< Image > s_image;

namespace Progression
{

extern bool g_converterMode;

namespace RenderSystem
{
    void InitSamplers();

    bool Init()
    {
        LOG( "SceneConstantBuffer.VP offset = ",             offsetof( SceneConstantBuffer, VP ) );
        LOG( "SceneConstantBuffer.cameraPos offset = ",      offsetof( SceneConstantBuffer, cameraPos ) );
        LOG( "SceneConstantBuffer.ambientColor offset = ",   offsetof( SceneConstantBuffer, ambientColor ) );
        LOG( "SceneConstantBuffer.dirLight offset = ",       offsetof( SceneConstantBuffer, dirLight ) );
        LOG( "SceneConstantBuffer.numPointLights offset = ", offsetof( SceneConstantBuffer, numPointLights ) );
        LOG( "SceneConstantBuffer.numSpotLights offset = ",  offsetof( SceneConstantBuffer, numSpotLights ) );
        InitTextureManager();
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
        s_gpuSceneConstantBuffers.resize( numImages );
        s_gpuPointLightBuffers.resize( numImages );
        for ( uint32_t i = 0; i < numImages; ++i )
        {
            s_gpuSceneConstantBuffers[i] = g_renderState.device.NewBuffer( sizeof( SceneConstantBuffer ),
                    BUFFER_TYPE_UNIFORM, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT );
            s_gpuPointLightBuffers[i] = g_renderState.device.NewBuffer( sizeof( PointLight ) * 1024,
                    BUFFER_TYPE_STORAGE, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT );
        }

        VkDescriptorPoolSize poolSize[3] = {};
        poolSize[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize[0].descriptorCount = numImages;
        poolSize[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSize[1].descriptorCount = numImages;
        poolSize[2].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize[2].descriptorCount = numImages;

        s_descriptorPool = g_renderState.device.NewDescriptorPool( 3, poolSize, 3 * numImages );

        std::vector< DescriptorSetLayoutData > descriptorSetData = simpleVert->reflectInfo.descriptorSetLayouts;
        descriptorSetData.insert( descriptorSetData.end(), simpleFrag->reflectInfo.descriptorSetLayouts.begin(), simpleFrag->reflectInfo.descriptorSetLayouts.end() );
        auto combined = CombineDescriptorSetLayouts( descriptorSetData );

        for ( size_t i = 0; i < combined.size(); ++i )
        {
            LOG( "combined[" , i, "].setNumber = ", combined[i].setNumber );
            for ( size_t b = 0; b < combined[i].bindings.size(); ++b )
            {
                LOG( "combined[" , i, "].binding[", b, "].binding = ", combined[i].bindings[b].binding );
                LOG( "combined[" , i, "].binding[", b, "].descriptorType = ", combined[i].bindings[b].descriptorType );
            }
        }

        s_descriptorSetLayouts = g_renderState.device.NewDescriptorSetLayouts( combined );
        sceneDescriptorSets = s_descriptorPool.NewDescriptorSets( numImages, s_descriptorSetLayouts[0] );
        textureDescriptorSets  = s_descriptorPool.NewDescriptorSets( numImages, s_descriptorSetLayouts[2] );

        s_image = ResourceManager::Get< Image >( "RENDER_SYSTEM_DUMMY_TEXTURE" );
        PG_ASSERT( s_image );
        VkDescriptorImageInfo imageInfo;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.sampler = s_image->sampler->GetHandle();
        imageInfo.imageView = s_image->GetTexture()->GetView();
        std::vector< VkDescriptorImageInfo > imageInfos( PG_MAX_NUM_TEXTURES, imageInfo );

        for ( size_t i = 0; i < numImages; i++ )
        {
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = s_gpuSceneConstantBuffers[i].GetHandle();
            bufferInfo.offset = 0;
            bufferInfo.range  = VK_WHOLE_SIZE;

            VkWriteDescriptorSet descriptorWrite[3] = {};
            descriptorWrite[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite[0].dstSet           = sceneDescriptorSets[i].GetHandle();
            descriptorWrite[0].dstBinding       = 0;
            descriptorWrite[0].dstArrayElement  = 0;
            descriptorWrite[0].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite[0].descriptorCount  = 1;
            descriptorWrite[0].pBufferInfo      = &bufferInfo;

            descriptorWrite[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite[1].dstSet           = textureDescriptorSets[i].GetHandle();
            descriptorWrite[1].dstBinding       = 0;
            descriptorWrite[1].dstArrayElement  = 0;
            descriptorWrite[1].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrite[1].descriptorCount  = static_cast< uint32_t >( imageInfos.size() );
            descriptorWrite[1].pImageInfo       = imageInfos.data();

            VkDescriptorBufferInfo bufferInfo2= {};
            bufferInfo2.buffer = s_gpuPointLightBuffers[i].GetHandle();
            bufferInfo2.offset = 0;
            bufferInfo2.range  = VK_WHOLE_SIZE;
            descriptorWrite[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite[2].dstSet           = sceneDescriptorSets[i].GetHandle();
            descriptorWrite[2].dstBinding       = 1;
            descriptorWrite[2].dstArrayElement  = 0;
            descriptorWrite[2].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrite[2].descriptorCount  = 1;
            descriptorWrite[2].pBufferInfo      = &bufferInfo2;

            vkUpdateDescriptorSets( g_renderState.device.GetHandle(), 3, descriptorWrite, 0, nullptr );
        }

        PipelineDescriptor pipelineDesc;
        pipelineDesc.renderPass             = &g_renderState.renderPass;
        pipelineDesc.descriptorSetLayouts   = s_descriptorSetLayouts;
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
            for ( size_t i = 0; i < g_renderState.swapChain.images.size(); ++i )
            {
                s_gpuSceneConstantBuffers[i].Free();
                s_gpuPointLightBuffers[i].Free();
            }
            for ( auto& layout : s_descriptorSetLayouts )
            {
                layout.Free();
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

        UpdateTextureDescriptors( textureDescriptorSets );

        // sceneConstantBuffer
        void* data = s_gpuSceneConstantBuffers[imageIndex].Map();
        SceneConstantBuffer scbuf;
        scbuf.VP             = scene->camera.GetVP();
        scbuf.cameraPos      = scene->camera.position;
        scbuf.ambientColor   = scene->ambientColor;
        scbuf.dirLight       = scene->directionalLight;
        scbuf.numPointLights = static_cast< uint32_t >( scene->pointLights.size() );
        scbuf.numSpotLights  = static_cast< uint32_t >( scene->spotLights.size() );
        memcpy( (char*)data, &scbuf, sizeof( SceneConstantBuffer ) );
        s_gpuSceneConstantBuffers[imageIndex].UnMap();

        data = s_gpuPointLightBuffers[imageIndex].Map();
        memcpy( (char*)data, scene->pointLights.data(), scene->pointLights.size() * sizeof( PointLight ) );
        s_gpuPointLightBuffers[imageIndex].UnMap();

        auto& cmdBuf = g_renderState.commandBuffers[imageIndex];
        cmdBuf.BeginRecording();
        cmdBuf.BeginRenderPass( g_renderState.renderPass, g_renderState.swapChainFramebuffers[imageIndex] );
        cmdBuf.BindRenderPipeline( s_pipeline );
        cmdBuf.BindDescriptorSets( 1, &sceneDescriptorSets[imageIndex], s_pipeline );
        cmdBuf.BindDescriptorSets( 1, &textureDescriptorSets[imageIndex], s_pipeline, 2 );

        scene->registry.view< ModelRenderer, Transform >().each( [&]( auto& modelRenderer, auto& transform )
        {
            auto M = transform.GetModelMatrix();
            auto N   = glm::transpose( glm::inverse( M ) );
            PerObjectConstantBuffer b;
            b.modelMatrix = M;
            b.normalMatrix = N;
            vkCmdPushConstants( cmdBuf.GetHandle(), s_pipeline.GetLayoutHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( PerObjectConstantBuffer ), &b );

            for ( size_t i = 0; i < modelRenderer.materials.size(); ++i )
            {
                const auto& mesh = modelRenderer.model->meshes[i];
                const auto& mat  = modelRenderer.materials[i];

                MaterialConstantBuffer mcbuf{};
                mcbuf.Ka = glm::vec4( mat->Ka, 0 );
                mcbuf.Kd = glm::vec4( mat->Kd, 0 );
                mcbuf.Ks = glm::vec4( mat->Ks, mat->Ns );
                mcbuf.diffuseTextureSlot = mat->map_Kd ? mat->map_Kd->GetTexture()->GetShaderSlot() : PG_INVALID_TEXTURE_INDEX;
                vkCmdPushConstants( cmdBuf.GetHandle(), s_pipeline.GetLayoutHandle(), VK_SHADER_STAGE_FRAGMENT_BIT, 128, sizeof( MaterialConstantBuffer ), &mcbuf );

                cmdBuf.BindVertexBuffer( mesh.vertexBuffer, 0, 0 );
                cmdBuf.BindVertexBuffer( mesh.vertexBuffer, mesh.GetNormalOffset(), 1 );
                cmdBuf.BindVertexBuffer( mesh.vertexBuffer, mesh.GetUVOffset(), 2 );
                cmdBuf.BindIndexBuffer(  mesh.indexBuffer,  mesh.GetIndexType() );
                cmdBuf.DrawIndexed( 0, mesh.GetNumIndices() );
            }
        });

        cmdBuf.EndRenderPass();
        cmdBuf.EndRecording();
        g_renderState.device.SubmitRenderCommands( 1, &cmdBuf );

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
