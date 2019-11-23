#include "graphics/render_system.hpp"
#include "core/animation_system.hpp"
#include "core/assert.hpp"
#include "core/scene.hpp"
#include "core/time.hpp"
#include "core/window.hpp"
#include "components/model_renderer.hpp"
#include "components/skinned_renderer.hpp"
#include "graphics/graphics_api.hpp"
#include "graphics/pg_to_vulkan_types.hpp"
#include "graphics/shader_c_shared/defines.h"
#include "graphics/texture_manager.hpp"
#include "resource/resource_manager.hpp"
#include "resource/image.hpp"
#include "resource/model.hpp"
#include "resource/shader.hpp"
#include "utils/logger.hpp"
#include <array>
#include <unordered_map>
#include "graphics/vulkan.hpp"
#include "graphics/shader_c_shared/structs.h"

using namespace Progression;
using namespace Gfx;

static std::unordered_map< std::string, Gfx::Sampler > s_samplers;

namespace Progression
{

namespace RenderSystem
{

    Gfx::Sampler* AddSampler( Gfx::SamplerDescriptor& desc )
    {
        auto it = s_samplers.find( desc.name );
        if ( it != s_samplers.end() )
        {
            return &it->second;
        }
        else
        {
            s_samplers[desc.name] = Gfx::g_renderState.device.NewSampler( desc );
            return &s_samplers[desc.name];
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
static Pipeline s_rigidModelPipeline;
static DescriptorPool s_descriptorPool;
static std::vector< DescriptorSetLayout > s_descriptorSetLayouts;
static std::vector< Gfx::Buffer > s_gpuSceneConstantBuffers;
static std::vector< Gfx::Buffer > s_gpuPointLightBuffers;
static std::vector< Gfx::Buffer > s_gpuSpotLightBuffers;
std::vector< DescriptorSet > sceneDescriptorSets;
std::vector< DescriptorSet > textureDescriptorSets;

struct OffScreenRenderData
{
    Gfx::RenderPass renderPass;
    Gfx::Texture colorAttachment;
    VkFramebuffer frameBuffer;
} offScreenRenderData;

#define MAX_NUM_POINT_LIGHTS 1024
#define MAX_NUM_SPOT_LIGHTS 256

namespace Progression
{
namespace RenderSystem
{

    bool Init()
    {
        s_window = GetMainWindow();

        uint32_t numImages = static_cast< uint32_t >( g_renderState.swapChain.images.size() );
        s_gpuSceneConstantBuffers.resize( numImages );
        s_gpuPointLightBuffers.resize( numImages );
        s_gpuSpotLightBuffers.resize( numImages );
        for ( uint32_t i = 0; i < numImages; ++i )
        {
            s_gpuSceneConstantBuffers[i] = g_renderState.device.NewBuffer( sizeof( SceneConstantBufferData ),
                    BUFFER_TYPE_UNIFORM, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT );
            s_gpuPointLightBuffers[i] = g_renderState.device.NewBuffer( sizeof( PointLight ) * MAX_NUM_POINT_LIGHTS,
                    BUFFER_TYPE_STORAGE, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT );
            s_gpuSpotLightBuffers[i] = g_renderState.device.NewBuffer( sizeof( SpotLight ) * MAX_NUM_SPOT_LIGHTS,
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
        
        auto rigidModelsVert       = ResourceManager::Get< Shader >( "rigidModelsVert" );
        auto forwardBlinnPhongFrag = ResourceManager::Get< Shader >( "forwardBlinnPhongFrag" );

        std::vector< DescriptorSetLayoutData > descriptorSetData = rigidModelsVert->reflectInfo.descriptorSetLayouts;
        descriptorSetData.insert( descriptorSetData.end(), forwardBlinnPhongFrag->reflectInfo.descriptorSetLayouts.begin(), forwardBlinnPhongFrag->reflectInfo.descriptorSetLayouts.end() );
        auto combined = CombineDescriptorSetLayouts( descriptorSetData );

        s_descriptorSetLayouts = g_renderState.device.NewDescriptorSetLayouts( combined );
        sceneDescriptorSets    = s_descriptorPool.NewDescriptorSets( numImages, s_descriptorSetLayouts[0] );
        textureDescriptorSets  = s_descriptorPool.NewDescriptorSets( numImages, s_descriptorSetLayouts[1] );

        auto dummyImage = ResourceManager::Get< Image >( "RENDER_SYSTEM_DUMMY_TEXTURE" );
        PG_ASSERT( dummyImage );
        VkDescriptorImageInfo imageInfo;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.sampler     = dummyImage->GetTexture()->GetSampler()->GetHandle();
        imageInfo.imageView   = dummyImage->GetTexture()->GetView();
        std::vector< VkDescriptorImageInfo > imageInfos( PG_MAX_NUM_TEXTURES, imageInfo );

        for ( size_t i = 0; i < numImages; i++ )
        {
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = s_gpuSceneConstantBuffers[i].GetHandle();
            bufferInfo.offset = 0;
            bufferInfo.range  = VK_WHOLE_SIZE;

            VkWriteDescriptorSet descriptorWrite[4] = {};
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

            g_renderState.device.UpdateDescriptorSets( 4, descriptorWrite );
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

        RenderPassDescriptor renderPassDesc;
        renderPassDesc.colorAttachmentDescriptors[0].format     = VulkanToPGPixelFormat( g_renderState.swapChain.imageFormat );
        renderPassDesc.colorAttachmentDescriptors[0].clearColor = glm::vec4( .2, .2, .2, 1 );
        renderPassDesc.depthAttachmentDescriptor.format         = PixelFormat::DEPTH_32_FLOAT;
        renderPassDesc.depthAttachmentDescriptor.loadAction     = LoadAction::CLEAR;
        renderPassDesc.depthAttachmentDescriptor.storeAction    = StoreAction::DONT_CARE;

        offScreenRenderData.renderPass = g_renderState.device.NewRenderPass( renderPassDesc );

        ImageDescriptor info;
        info.type    = ImageType::TYPE_2D;
        info.format  = VulkanToPGPixelFormat( g_renderState.swapChain.imageFormat );
        info.width   = g_renderState.swapChain.extent.width;
        info.height  = g_renderState.swapChain.extent.height;
        info.sampler = "nearest_clamped_nearest";
        offScreenRenderData.colorAttachment = g_renderState.device.NewTexture( info );
        TransitionImageLayout( offScreenRenderData.colorAttachment.GetHandle(), g_renderState.swapChain.imageFormat,
                               VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );

        VkImageView attachments[2];
        attachments[0] = offScreenRenderData.colorAttachment.GetView();
        attachments[1] = g_renderState.depthTex.GetView();

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = offScreenRenderData.renderPass.GetHandle();
        framebufferInfo.attachmentCount = 2;
        framebufferInfo.pAttachments    = attachments;
        framebufferInfo.width           = offScreenRenderData.colorAttachment.GetWidth();
        framebufferInfo.height          = offScreenRenderData.colorAttachment.GetHeight();
        framebufferInfo.layers          = 1;

        if ( vkCreateFramebuffer( g_renderState.device.GetHandle(), &framebufferInfo, nullptr, &offScreenRenderData.frameBuffer ) != VK_SUCCESS )
        {
            LOG_ERR( "Could not create offscreen framebuffer" );
            return false;
        }

        PipelineDescriptor pipelineDesc;
        pipelineDesc.renderPass             = &offScreenRenderData.renderPass;
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

        return true;
    }

    void Shutdown()
    {
        g_renderState.device.WaitForIdle();

        s_descriptorPool.Free();
        for ( size_t i = 0; i < g_renderState.swapChain.images.size(); ++i )
        {
            s_gpuSceneConstantBuffers[i].Free();
            s_gpuPointLightBuffers[i].Free();
            s_gpuSpotLightBuffers[i].Free();
        }
        for ( auto& layout : s_descriptorSetLayouts )
        {
            layout.Free();
        }
        s_rigidModelPipeline.Free();
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

        TextureManager::UpdateDescriptors( textureDescriptorSets );

        // sceneConstantBuffer
        SceneConstantBufferData scbuf;
        scbuf.V              = scene->camera.GetV();
        scbuf.P              = scene->camera.GetP();
        scbuf.VP             = scene->camera.GetVP();
        scbuf.cameraPos      = glm::vec4( scene->camera.position, 0 );
        scbuf.ambientColor   = glm::vec4( scene->ambientColor, 0 );
        scbuf.dirLight       = scene->directionalLight;
        scbuf.numPointLights = static_cast< uint32_t >( scene->pointLights.size() );
        scbuf.numSpotLights  = static_cast< uint32_t >( scene->spotLights.size() );
        s_gpuSceneConstantBuffers[imageIndex].Map();
        memcpy( s_gpuSceneConstantBuffers[imageIndex].MappedPtr(), &scbuf, sizeof( SceneConstantBufferData ) );
        s_gpuSceneConstantBuffers[imageIndex].UnMap();

        s_gpuPointLightBuffers[imageIndex].Map();
        memcpy( s_gpuPointLightBuffers[imageIndex].MappedPtr(), scene->pointLights.data(), scene->pointLights.size() * sizeof( PointLight ) );
        s_gpuPointLightBuffers[imageIndex].UnMap();

        s_gpuSpotLightBuffers[imageIndex].Map();
        memcpy( s_gpuSpotLightBuffers[imageIndex].MappedPtr(), scene->spotLights.data(), scene->spotLights.size() * sizeof( SpotLight ) );
        s_gpuSpotLightBuffers[imageIndex].UnMap();

        auto& cmdBuf = g_renderState.commandBuffers[imageIndex];
        cmdBuf.BeginRecording();
        // cmdBuf.BeginRenderPass( g_renderState.renderPass, g_renderState.swapChainFramebuffers[imageIndex] );
        cmdBuf.BeginRenderPass( offScreenRenderData.renderPass, offScreenRenderData.frameBuffer );
        cmdBuf.BindRenderPipeline( s_rigidModelPipeline );
        cmdBuf.BindDescriptorSets( 1, &sceneDescriptorSets[imageIndex], s_rigidModelPipeline, PG_SCENE_CONSTANT_BUFFER_SET );
        cmdBuf.BindDescriptorSets( 1, &textureDescriptorSets[imageIndex], s_rigidModelPipeline, PG_2D_TEXTURES_SET );

        scene->registry.view< ModelRenderer, Transform >().each( [&]( ModelRenderer& modelRenderer, Transform& transform )
        {
            const auto& model = modelRenderer.model;
            auto M = transform.GetModelMatrix();
            auto N = glm::transpose( glm::inverse( M ) );
            ObjectConstantBufferData b;
            b.M = M;
            b.N = N;
            vkCmdPushConstants( cmdBuf.GetHandle(), s_rigidModelPipeline.GetLayoutHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( ObjectConstantBufferData ), &b );

            cmdBuf.BindVertexBuffer( model->vertexBuffer, 0, 0 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetNormalOffset(), 1 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetUVOffset(), 2 );
            cmdBuf.BindIndexBuffer(  model->indexBuffer, model->GetIndexType() );

            for ( size_t i = 0; i < model->meshes.size(); ++i )
            {
                const auto& mesh = modelRenderer.model->meshes[i];
                // const auto& mat  = modelRenderer.materials[i];
                const auto& mat  = modelRenderer.materials[mesh.materialIndex];

                MaterialConstantBufferData mcbuf{};
                mcbuf.Ka = glm::vec4( mat->Ka, 0 );
                mcbuf.Kd = glm::vec4( mat->Kd, 0 );
                mcbuf.Ks = glm::vec4( mat->Ks, mat->Ns );
                mcbuf.diffuseTexIndex = mat->map_Kd ? mat->map_Kd->GetTexture()->GetShaderSlot() : PG_INVALID_TEXTURE_INDEX;
                vkCmdPushConstants( cmdBuf.GetHandle(), s_rigidModelPipeline.GetLayoutHandle(), VK_SHADER_STAGE_FRAGMENT_BIT, PG_MATERIAL_PUSH_CONSTANT_OFFSET, sizeof( MaterialConstantBufferData ), &mcbuf );

                cmdBuf.DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
            }
        });

        AnimationSystem::UploadToGpu( scene, imageIndex );

        auto& animPipeline = AnimationSystem::renderData.animatedPipeline;
        cmdBuf.BindRenderPipeline( animPipeline );
        cmdBuf.BindDescriptorSets( 1, &sceneDescriptorSets[imageIndex], animPipeline, PG_SCENE_CONSTANT_BUFFER_SET );
        cmdBuf.BindDescriptorSets( 1, &textureDescriptorSets[imageIndex], animPipeline, PG_2D_TEXTURES_SET );
        cmdBuf.BindDescriptorSets( 1, &AnimationSystem::renderData.animationBonesDescriptorSets[imageIndex], animPipeline, PG_BONE_TRANSFORMS_SET );
        scene->registry.view< Animator, SkinnedRenderer, Transform >().each( [&]( Animator& animator, SkinnedRenderer& renderer, Transform& transform )
        {
            const auto& model = renderer.model;
            auto M            = transform.GetModelMatrix();
            auto N            = glm::transpose( glm::inverse( M ) );

            AnimatedObjectConstantBufferData b;
            b.M = M;
            b.N = N;
            b.boneTransformIdx = animator.GetTransformSlot();
            vkCmdPushConstants( cmdBuf.GetHandle(), animPipeline.GetLayoutHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( AnimatedObjectConstantBufferData ), &b );

            cmdBuf.BindVertexBuffer( model->vertexBuffer, 0, 0 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetNormalOffset(), 1 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetUVOffset(), 2 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetBlendWeightOffset(), 3 );
            cmdBuf.BindIndexBuffer(  model->indexBuffer, model->GetIndexType() );

            for ( size_t i = 0; i < renderer.model->meshes.size(); ++i )
            {
                const auto& mesh = model->meshes[i];
                const auto& mat  = renderer.materials[mesh.materialIndex];

                MaterialConstantBufferData mcbuf{};
                mcbuf.Ka = glm::vec4( mat->Ka, 0 );
                mcbuf.Kd = glm::vec4( mat->Kd, 0 );
                mcbuf.Ks = glm::vec4( mat->Ks, mat->Ns );
                mcbuf.diffuseTexIndex = mat->map_Kd ? mat->map_Kd->GetTexture()->GetShaderSlot() : PG_INVALID_TEXTURE_INDEX;
                vkCmdPushConstants( cmdBuf.GetHandle(), animPipeline.GetLayoutHandle(), VK_SHADER_STAGE_FRAGMENT_BIT, PG_MATERIAL_PUSH_CONSTANT_OFFSET, sizeof( MaterialConstantBufferData ), &mcbuf );
                
                cmdBuf.DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
            }
        });

        cmdBuf.EndRenderPass();

        // cmdBuf.BeginRenderPass( g_renderState.renderPass, g_renderState.swapChainFramebuffers[imageIndex] );
        // cmdBuf.EndRenderPass();

        cmdBuf.EndRecording();
        g_renderState.device.SubmitRenderCommands( 1, &cmdBuf );

        g_renderState.device.SubmitFrame( imageIndex );
    } 

    void InitSamplers()
    {
        SamplerDescriptor samplerDesc;

        samplerDesc.name = "nearest_clamped_nearest";
        samplerDesc.minFilter = FilterMode::NEAREST;
        samplerDesc.magFilter = FilterMode::NEAREST;
        samplerDesc.mipFilter = MipFilterMode::NEAREST;
        samplerDesc.wrapModeU = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeV = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeW = WrapMode::CLAMP_TO_EDGE;
        AddSampler( samplerDesc );

        samplerDesc.name = "linear_clamped_linear";
        samplerDesc.mipFilter = MipFilterMode::LINEAR;
        samplerDesc.minFilter = FilterMode::LINEAR;
        samplerDesc.magFilter = FilterMode::LINEAR;
        AddSampler( samplerDesc );

        samplerDesc.name = "linear_repeat_linear";
        samplerDesc.wrapModeU = WrapMode::REPEAT;
        samplerDesc.wrapModeV = WrapMode::REPEAT;
        samplerDesc.wrapModeW = WrapMode::REPEAT;
        AddSampler( samplerDesc );
    }

    void FreeSamplers()
    {
        for ( auto& [name, sampler] : s_samplers )
        {
            sampler.Free();
        }
    }

} // namespace RenderSystem
} // namespace Progression
