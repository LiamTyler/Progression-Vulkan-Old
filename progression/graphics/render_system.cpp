#include "graphics/render_system.hpp"
#include "core/assert.hpp"
#include "core/scene.hpp"
#include "core/time.hpp"
#include "core/window.hpp"
#include "components/model_renderer.hpp"
#include "components/skinned_renderer.hpp"
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
static Pipeline s_rigidModelPipeline;
static Pipeline s_animatedModelPipeline;
static DescriptorPool s_descriptorPool;
static std::vector< DescriptorSetLayout > s_descriptorSetLayouts;
static std::vector< Progression::Gfx::Buffer > s_gpuSceneConstantBuffers;
static std::vector< Progression::Gfx::Buffer > s_gpuPointLightBuffers;
static std::vector< Progression::Gfx::Buffer > s_gpuSpotLightBuffers;
static std::vector< Progression::Gfx::Buffer > s_gpuBoneBuffers;
std::vector< DescriptorSet > sceneDescriptorSets;
std::vector< DescriptorSet > textureDescriptorSets;
std::vector< DescriptorSet > animationBonesDescriptorSets;
static std::shared_ptr< Image > s_image;

#define MAX_NUM_POINT_LIGHTS 1024
#define MAX_NUM_SPOT_LIGHTS 256

namespace Progression
{

extern bool g_converterMode;

namespace RenderSystem
{
    void InitSamplers();

    bool Init()
    {
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
        
        uint32_t numImages = static_cast< uint32_t >( g_renderState.swapChain.images.size() );
        s_gpuSceneConstantBuffers.resize( numImages );
        s_gpuPointLightBuffers.resize( numImages );
        s_gpuSpotLightBuffers.resize( numImages );
        s_gpuBoneBuffers.resize( numImages );
        for ( uint32_t i = 0; i < numImages; ++i )
        {
            s_gpuSceneConstantBuffers[i] = g_renderState.device.NewBuffer( sizeof( SceneConstantBuffer ),
                    BUFFER_TYPE_UNIFORM, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT );
            s_gpuPointLightBuffers[i] = g_renderState.device.NewBuffer( sizeof( PointLight ) * MAX_NUM_POINT_LIGHTS,
                    BUFFER_TYPE_STORAGE, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT );
            s_gpuSpotLightBuffers[i] = g_renderState.device.NewBuffer( sizeof( SpotLight ) * MAX_NUM_SPOT_LIGHTS,
                    BUFFER_TYPE_STORAGE, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT );
            s_gpuBoneBuffers[i] = g_renderState.device.NewBuffer( sizeof( glm::mat4 ) * 1000,
                    BUFFER_TYPE_STORAGE, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT );
        }

        VkDescriptorPoolSize poolSize[3] = {};
        poolSize[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize[0].descriptorCount = numImages;
        poolSize[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSize[1].descriptorCount = 3 * numImages;
        poolSize[2].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize[2].descriptorCount = numImages;

        s_descriptorPool = g_renderState.device.NewDescriptorPool( 3, poolSize, 3 * numImages );
        
        ResourceManager::LoadFastFile( PG_RESOURCE_DIR "cache/fastfiles/resource.txt.ff" );
        auto rigidModelsVert       = ResourceManager::Get< Shader >( "rigidModelsVert" );
        auto animatedModelsVert    = ResourceManager::Get< Shader >( "animatedModelsVert" );
        auto forwardBlinnPhongFrag = ResourceManager::Get< Shader >( "forwardBlinnPhongFrag" );

        std::vector< DescriptorSetLayoutData > descriptorSetData = animatedModelsVert->reflectInfo.descriptorSetLayouts;
        descriptorSetData.insert( descriptorSetData.end(), forwardBlinnPhongFrag->reflectInfo.descriptorSetLayouts.begin(), forwardBlinnPhongFrag->reflectInfo.descriptorSetLayouts.end() );
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

        s_descriptorSetLayouts       = g_renderState.device.NewDescriptorSetLayouts( combined );
        sceneDescriptorSets          = s_descriptorPool.NewDescriptorSets( numImages, s_descriptorSetLayouts[0] );
        textureDescriptorSets        = s_descriptorPool.NewDescriptorSets( numImages, s_descriptorSetLayouts[1] );
        animationBonesDescriptorSets = s_descriptorPool.NewDescriptorSets( numImages, s_descriptorSetLayouts[2] );

        s_image = ResourceManager::Get< Image >( "RENDER_SYSTEM_DUMMY_TEXTURE" );
        PG_ASSERT( s_image );
        VkDescriptorImageInfo imageInfo;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.sampler     = s_image->sampler->GetHandle();
        imageInfo.imageView   = s_image->GetTexture()->GetView();
        std::vector< VkDescriptorImageInfo > imageInfos( PG_MAX_NUM_TEXTURES, imageInfo );

        for ( size_t i = 0; i < numImages; i++ )
        {
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = s_gpuSceneConstantBuffers[i].GetHandle();
            bufferInfo.offset = 0;
            bufferInfo.range  = VK_WHOLE_SIZE;

            VkWriteDescriptorSet descriptorWrite[5] = {};
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

            VkDescriptorBufferInfo bufferInfo2 = {};
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

            VkDescriptorBufferInfo bufferInfo3 = {};
            bufferInfo3.buffer = s_gpuSpotLightBuffers[i].GetHandle();
            bufferInfo3.offset = 0;
            bufferInfo3.range  = VK_WHOLE_SIZE;
            descriptorWrite[3].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite[3].dstSet           = sceneDescriptorSets[i].GetHandle();
            descriptorWrite[3].dstBinding       = 2;
            descriptorWrite[3].dstArrayElement  = 0;
            descriptorWrite[3].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrite[3].descriptorCount  = 1;
            descriptorWrite[3].pBufferInfo      = &bufferInfo3;

            VkDescriptorBufferInfo boneDataBufferInfo = {};
            boneDataBufferInfo.buffer = s_gpuBoneBuffers[i].GetHandle();
            boneDataBufferInfo.offset = 0;
            boneDataBufferInfo.range  = VK_WHOLE_SIZE;
            descriptorWrite[4].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite[4].dstSet           = animationBonesDescriptorSets[i].GetHandle();
            descriptorWrite[4].dstBinding       = 0;
            descriptorWrite[4].dstArrayElement  = 0;
            descriptorWrite[4].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrite[4].descriptorCount  = 1;
            descriptorWrite[4].pBufferInfo      = &boneDataBufferInfo;

            vkUpdateDescriptorSets( g_renderState.device.GetHandle(), 5, descriptorWrite, 0, nullptr );
        }
 
        VertexBindingDescriptor bindingDesc[5];
        bindingDesc[0].binding   = 0;
        bindingDesc[0].stride    = sizeof( glm::vec3 );
        bindingDesc[0].inputRate = VertexInputRate::PER_VERTEX;
        bindingDesc[1].binding   = 1;
        bindingDesc[1].stride    = sizeof( glm::vec3 );
        bindingDesc[1].inputRate = VertexInputRate::PER_VERTEX;
        bindingDesc[2].binding   = 2;
        bindingDesc[2].stride    = sizeof( glm::vec2 );
        bindingDesc[2].inputRate = VertexInputRate::PER_VERTEX;
        bindingDesc[3].binding   = 3;
        bindingDesc[3].stride    = 2 * sizeof( glm::vec4 );
        bindingDesc[3].inputRate = VertexInputRate::PER_VERTEX;
        bindingDesc[4].binding   = 4;
        bindingDesc[4].stride    = 1 * sizeof( glm::uvec4 );
        bindingDesc[4].inputRate = VertexInputRate::PER_VERTEX;

        std::array< VertexAttributeDescriptor, 5 > attribDescs;
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
        attribDescs[3].binding  = 3;
        attribDescs[3].location = 3;
        attribDescs[3].format   = BufferDataType::FLOAT4;
        attribDescs[3].offset   = 0;
        attribDescs[4].binding  = 3;
        attribDescs[4].location = 4;
        attribDescs[4].format   = BufferDataType::UINT4;
        attribDescs[4].offset   = sizeof( glm::vec4 );

        PipelineDescriptor pipelineDesc;
        pipelineDesc.renderPass             = &g_renderState.renderPass;
        pipelineDesc.descriptorSetLayouts   = s_descriptorSetLayouts;
        pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 3, bindingDesc, 3, attribDescs.data() );
        pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
        pipelineDesc.viewport               = FullScreenViewport();
        pipelineDesc.scissor                = FullScreenScissor();
        pipelineDesc.shaders[0]             = rigidModelsVert.get();
        pipelineDesc.shaders[1]             = forwardBlinnPhongFrag.get();
        pipelineDesc.numShaders             = 2;

        s_rigidModelPipeline = g_renderState.device.NewPipeline( pipelineDesc );
        if ( !s_rigidModelPipeline )
        {
            LOG_ERR( "Could not create rigid model pipeline" );
            return false;
        }

        pipelineDesc.descriptorSetLayouts   = s_descriptorSetLayouts;
        pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 4, bindingDesc, 5, attribDescs.data() );
        pipelineDesc.shaders[0]             = animatedModelsVert.get();
        pipelineDesc.numShaders             = 2;

        s_animatedModelPipeline = g_renderState.device.NewPipeline( pipelineDesc );
        if ( !s_animatedModelPipeline )
        {
            LOG_ERR( "Could not create animated model pipeline" );
            return false;
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
            for ( size_t i = 0; i < g_renderState.swapChain.images.size(); ++i )
            {
                s_gpuSceneConstantBuffers[i].Free();
                s_gpuPointLightBuffers[i].Free();
                s_gpuSpotLightBuffers[i].Free();
                s_gpuBoneBuffers[i].Free();
            }
            for ( auto& layout : s_descriptorSetLayouts )
            {
                layout.Free();
            }
            s_rigidModelPipeline.Free();
            s_animatedModelPipeline.Free();
        }

        VulkanShutdown();
    }

    void Render( Scene* scene )
    {
        PG_ASSERT( scene != nullptr );
        PG_ASSERT( scene->pointLights.size() < MAX_NUM_POINT_LIGHTS && scene->spotLights.size() < MAX_NUM_SPOT_LIGHTS );
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

        data = s_gpuSpotLightBuffers[imageIndex].Map();
        memcpy( (char*)data, scene->spotLights.data(), scene->spotLights.size() * sizeof( SpotLight ) );
        s_gpuSpotLightBuffers[imageIndex].UnMap();

        auto& cmdBuf = g_renderState.commandBuffers[imageIndex];
        cmdBuf.BeginRecording();
        cmdBuf.BeginRenderPass( g_renderState.renderPass, g_renderState.swapChainFramebuffers[imageIndex] );
        cmdBuf.BindRenderPipeline( s_rigidModelPipeline );
        cmdBuf.BindDescriptorSets( 1, &sceneDescriptorSets[imageIndex], s_rigidModelPipeline, 0 );
        cmdBuf.BindDescriptorSets( 1, &textureDescriptorSets[imageIndex], s_rigidModelPipeline, 1 );

        scene->registry.view< ModelRenderer, Transform >().each( [&]( ModelRenderer& modelRenderer, Transform& transform )
        {
            // LOG( "Drawing model: ", modelRenderer.model->name );
            auto M = transform.GetModelMatrix();
            auto N   = glm::transpose( glm::inverse( M ) );
            PerObjectConstantBuffer b;
            b.modelMatrix = M;
            b.normalMatrix = N;
            vkCmdPushConstants( cmdBuf.GetHandle(), s_rigidModelPipeline.GetLayoutHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( PerObjectConstantBuffer ), &b );

            for ( size_t i = 0; i < modelRenderer.materials.size(); ++i )
            {
                const auto& mesh = modelRenderer.model->meshes[i];
                const auto& mat  = modelRenderer.materials[i];

                MaterialConstantBuffer mcbuf{};
                mcbuf.Ka = glm::vec4( mat->Ka, 0 );
                mcbuf.Kd = glm::vec4( mat->Kd, 0 );
                mcbuf.Ks = glm::vec4( mat->Ks, mat->Ns );
                mcbuf.diffuseTextureSlot = mat->map_Kd ? mat->map_Kd->GetTexture()->GetShaderSlot() : PG_INVALID_TEXTURE_INDEX;
                vkCmdPushConstants( cmdBuf.GetHandle(), s_rigidModelPipeline.GetLayoutHandle(), VK_SHADER_STAGE_FRAGMENT_BIT, 128, sizeof( MaterialConstantBuffer ), &mcbuf );

                cmdBuf.BindVertexBuffer( mesh.vertexBuffer, 0, 0 );
                cmdBuf.BindVertexBuffer( mesh.vertexBuffer, mesh.GetNormalOffset(), 1 );
                cmdBuf.BindVertexBuffer( mesh.vertexBuffer, mesh.GetUVOffset(), 2 );
                cmdBuf.BindIndexBuffer(  mesh.indexBuffer,  mesh.GetIndexType() );
                cmdBuf.DrawIndexed( 0, mesh.GetNumIndices() );
            }
        });

        cmdBuf.BindRenderPipeline( s_animatedModelPipeline );
        //cmdBuf.BindDescriptorSets( 1, &sceneDescriptorSets[imageIndex], s_animatedModelPipeline, 0 );
        //cmdBuf.BindDescriptorSets( 1, &textureDescriptorSets[imageIndex], s_animatedModelPipeline, 1 );
        cmdBuf.BindDescriptorSets( 1, &animationBonesDescriptorSets[imageIndex], s_animatedModelPipeline, 2 );
        scene->registry.view< SkinnedRenderer, Transform >().each( [&]( SkinnedRenderer& renderer, Transform& transform )
        {
            const auto& model = renderer.model;
            auto M            = transform.GetModelMatrix();
            auto N            = glm::transpose( glm::inverse( M ) );

            PerObjectConstantBuffer b;
            b.modelMatrix  = M;
            b.normalMatrix = N;
            vkCmdPushConstants( cmdBuf.GetHandle(), s_animatedModelPipeline.GetLayoutHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( PerObjectConstantBuffer ), &b );

            // NOTE: This wont work for more than one model, would to copy all transforms to a big buffer and then do all the draws
            std::vector< glm::mat4 > boneTransforms; //( model->joints.size(), M );
            model->GetCurrentPose( boneTransforms );
            data = s_gpuBoneBuffers[imageIndex].Map();
            memcpy( (char*)data, boneTransforms.data(), boneTransforms.size() * sizeof( glm::mat4 ) );
            s_gpuBoneBuffers[imageIndex].UnMap();

            for ( size_t i = 0; i < renderer.model->meshes.size(); ++i )
            {
                const auto& mesh = model->meshes[i];
                const auto& mat  = model->materials[mesh.materialIndex];

                MaterialConstantBuffer mcbuf{};
                mcbuf.Ka = glm::vec4( mat->Ka, 0 );
                mcbuf.Kd = glm::vec4( mat->Kd, 0 );
                mcbuf.Ks = glm::vec4( mat->Ks, mat->Ns );
                mcbuf.diffuseTextureSlot = mat->map_Kd ? mat->map_Kd->GetTexture()->GetShaderSlot() : PG_INVALID_TEXTURE_INDEX;
                vkCmdPushConstants( cmdBuf.GetHandle(), s_animatedModelPipeline.GetLayoutHandle(), VK_SHADER_STAGE_FRAGMENT_BIT, 128, sizeof( MaterialConstantBuffer ), &mcbuf );

                cmdBuf.BindVertexBuffer( model->vertexBuffer, 0, 0 );
                cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetNormalOffset(), 1 );
                cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetUVOffset(), 2 );
                cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetBlendWeightOffset(), 3 );
                cmdBuf.BindIndexBuffer(  model->indexBuffer, model->GetIndexType() );
                cmdBuf.DrawIndexed( mesh.GetStartIndex(), mesh.GetNumIndices(), mesh.GetStartVertex() );
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
