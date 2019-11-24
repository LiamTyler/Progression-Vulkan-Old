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
	Gfx::Pipeline postPorcessingPipeline;
    Gfx::Buffer quadBuffer;
    Gfx::DescriptorSet textureToProcess;
    std::vector< Gfx::DescriptorSetLayout > textureToProcessLayout;
} offScreenRenderData;

bool CreateOffScreenRenderPass()
{
    RenderPassDescriptor desc;
    desc.colorAttachmentDescriptors[0].format     = VulkanToPGPixelFormat( g_renderState.swapChain.imageFormat );
    desc.colorAttachmentDescriptors[0].clearColor = glm::vec4( .2, .2, .2, 1 );
    desc.depthAttachmentDescriptor.format         = PixelFormat::DEPTH_32_FLOAT;
    desc.depthAttachmentDescriptor.loadAction     = LoadAction::CLEAR;
    desc.depthAttachmentDescriptor.storeAction    = StoreAction::STORE;
    auto& pass = offScreenRenderData.renderPass;
    pass.desc     = desc;
    pass.m_device = g_renderState.device.GetHandle();

    std::vector< VkAttachmentDescription > attachments;
    std::vector< VkAttachmentReference > attachmentRefs;

    const auto& attach = desc.colorAttachmentDescriptors[0];
    attachments.push_back( {} );
    attachments[0].format         = PGToVulkanPixelFormat( attach.format );
    attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp         = PGToVulkanLoadAction( attach.loadAction );
    attachments[0].storeOp        = PGToVulkanStoreAction( attach.storeAction );
    attachments[0].stencilLoadOp  = PGToVulkanLoadAction( LoadAction::DONT_CARE );
    attachments[0].stencilStoreOp = PGToVulkanStoreAction( StoreAction::DONT_CARE );
    attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    attachmentRefs.push_back( {} );
    attachmentRefs[0].attachment = 0;
    attachmentRefs[0].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format         = PGToVulkanPixelFormat( desc.depthAttachmentDescriptor.format );
    depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp         = PGToVulkanLoadAction( desc.depthAttachmentDescriptor.loadAction );
    depthAttachment.storeOp        = PGToVulkanStoreAction( desc.depthAttachmentDescriptor.storeAction );
    depthAttachment.stencilLoadOp  = PGToVulkanLoadAction( LoadAction::DONT_CARE );
    depthAttachment.stencilStoreOp = PGToVulkanStoreAction( StoreAction::DONT_CARE );
    depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments.push_back( depthAttachment );

    VkSubpassDescription subpass    = {};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = attachmentRefs.data();
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast< uint32_t >( attachments.size() );
    renderPassInfo.pAttachments    = attachments.data();
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies   = &dependency;

    if ( vkCreateRenderPass( g_renderState.device.GetHandle(), &renderPassInfo, nullptr, &pass.m_handle ) != VK_SUCCESS )
    {
        pass.m_handle = VK_NULL_HANDLE;
        LOG_ERR( "Could not create render pass" );
        return false;
    }

    return true;
}

struct ShadowRenderData
{
    Gfx::RenderPass renderPass;
    Gfx::Texture depthAttachment;
    VkFramebuffer frameBuffer;
    Gfx::Pipeline pipeline;
    Gfx::DescriptorSet descSet;
    std::vector< Gfx::DescriptorSetLayout > descSetLayouts;
} directionalShadow;

#define MAX_NUM_POINT_LIGHTS 1024
#define MAX_NUM_SPOT_LIGHTS 256
#define DIRECTIONAL_SHADOW_MAP_RESOLUTION 1024

namespace Progression
{
namespace RenderSystem
{

    bool Init()
    {
        s_window = GetMainWindow();
        
		// Post Processing
        if ( !CreateOffScreenRenderPass() )
        {
            return false;
        }

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
        poolSize[2].descriptorCount = numImages + 1;

        s_descriptorPool = g_renderState.device.NewDescriptorPool( 3, poolSize, 3 * numImages );
        
        auto rigidModelsVert		= ResourceManager::Get< Shader >( "rigidModelsVert" );
        auto forwardBlinnPhongFrag  = ResourceManager::Get< Shader >( "forwardBlinnPhongFrag" );

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

        ImageDescriptor info;
        info.type    = ImageType::TYPE_2D;
        info.format  = VulkanToPGPixelFormat( g_renderState.swapChain.imageFormat );
        info.width   = g_renderState.swapChain.extent.width;
        info.height  = g_renderState.swapChain.extent.height;
        info.sampler = "nearest_clamped_nearest";
        info.usage   = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        offScreenRenderData.colorAttachment = g_renderState.device.NewTexture( info, false );

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



		// Post Processing

        // Post Processing shaders
        auto postProcessingVert = ResourceManager::Get< Shader >("postProcessVert");
        auto postProcessingFrag = ResourceManager::Get< Shader >("postProcessFrag");

        descriptorSetData = postProcessingVert->reflectInfo.descriptorSetLayouts;
        descriptorSetData.insert(descriptorSetData.end(), postProcessingFrag->reflectInfo.descriptorSetLayouts.begin(), postProcessingFrag->reflectInfo.descriptorSetLayouts.end());
        combined = CombineDescriptorSetLayouts(descriptorSetData);

        offScreenRenderData.textureToProcessLayout = g_renderState.device.NewDescriptorSetLayouts(combined);
        offScreenRenderData.textureToProcess = s_descriptorPool.NewDescriptorSets(1, offScreenRenderData.textureToProcessLayout[0])[0];


		VertexBindingDescriptor postProcescsingBindingDesc[1];
        postProcescsingBindingDesc[0].binding = 0;
        postProcescsingBindingDesc[0].stride = sizeof(glm::vec3);
        postProcescsingBindingDesc[0].inputRate = VertexInputRate::PER_VERTEX;

		std::array< VertexAttributeDescriptor, 1 > postProcescsingAttribDescs;
        postProcescsingAttribDescs[0].binding = 0;
        postProcescsingAttribDescs[0].location = 0;
        postProcescsingAttribDescs[0].format = BufferDataType::FLOAT3;
        postProcescsingAttribDescs[0].offset = 0;

		PipelineDescriptor postProcessingPipelineDesc;
        postProcessingPipelineDesc.renderPass	          = &g_renderState.renderPass;
        postProcessingPipelineDesc.descriptorSetLayouts   = offScreenRenderData.textureToProcessLayout;
        postProcessingPipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create(1, postProcescsingBindingDesc, 1, postProcescsingAttribDescs.data());
        postProcessingPipelineDesc.rasterizerInfo.winding = WindingOrder::CLOCKWISE; // TODO: Why is this clockwise??
        postProcessingPipelineDesc.viewport               = FullScreenViewport();
        postProcessingPipelineDesc.scissor                = FullScreenScissor();
        postProcessingPipelineDesc.shaders[0]	          = postProcessingVert.get();
        postProcessingPipelineDesc.shaders[1]			  = postProcessingFrag.get();
        postProcessingPipelineDesc.numShaders             = 2;

		offScreenRenderData.postPorcessingPipeline = g_renderState.device.NewPipeline( postProcessingPipelineDesc );
		if ( !offScreenRenderData.postPorcessingPipeline )
		{
			LOG_ERR("Could not create post processing pipeline");
			return false;
		}

        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.sampler = offScreenRenderData.colorAttachment.GetSampler()->GetHandle();
        imageInfo.imageView = offScreenRenderData.colorAttachment.GetView();

        VkWriteDescriptorSet descriptorWrite[1] = {};
        descriptorWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite[0].dstSet = offScreenRenderData.textureToProcess.GetHandle();
        descriptorWrite[0].dstBinding = 0;
        descriptorWrite[0].dstArrayElement = 0;
        descriptorWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite[0].descriptorCount = 1;
        descriptorWrite[0].pImageInfo = &imageInfo;

        g_renderState.device.UpdateDescriptorSets(1, descriptorWrite);

        glm::vec3 quadVerts[] =
        {
            glm::vec3( -1, -1, 0 ),
            glm::vec3(  1,  1, 0 ),
            glm::vec3( -1,  1, 0 ),
            glm::vec3( -1, -1, 0 ),
            glm::vec3(  1, -1, 0 ),
            glm::vec3(  1,  1, 0 )
        };

        offScreenRenderData.quadBuffer = g_renderState.device.NewBuffer(sizeof(quadVerts), quadVerts, BUFFER_TYPE_VERTEX, MEMORY_TYPE_DEVICE_LOCAL );




        // Shadow Mapping for Direction Light

        auto directionalShadowVert = ResourceManager::Get< Shader >("directionalShadowVert");

        descriptorSetData = directionalShadowVert->reflectInfo.descriptorSetLayouts;

        // TODO: Some wonky stuff might be happening here, check back after we know which descSet stuff we need for shadow maps (uniform view)
        directionalShadow.descSetLayouts = g_renderState.device.NewDescriptorSetLayouts(descriptorSetData);
        directionalShadow.descSet = s_descriptorPool.NewDescriptorSets(1, directionalShadow.descSetLayouts[0])[0];

        
        VertexBindingDescriptor directionalShadowBindingDesc[1];
        directionalShadowBindingDesc[0].binding = 0;
        directionalShadowBindingDesc[0].stride = sizeof(glm::vec3);
        directionalShadowBindingDesc[0].inputRate = VertexInputRate::PER_VERTEX;

        std::array< VertexAttributeDescriptor, 1 > directionalShadowAttribDescs;
        directionalShadowAttribDescs[0].binding = 0;
        directionalShadowAttribDescs[0].location = 0;
        directionalShadowAttribDescs[0].format = BufferDataType::FLOAT3;
        directionalShadowAttribDescs[0].offset = 0;

        PipelineDescriptor directionalShadowPipelineDesc;
        directionalShadowPipelineDesc.renderPass = &g_renderState.renderPass;
        directionalShadowPipelineDesc.descriptorSetLayouts = directionalShadow.descSetLayouts;
        directionalShadowPipelineDesc.vertexDescriptor = VertexInputDescriptor::Create(1, directionalShadowBindingDesc, 1, directionalShadowAttribDescs.data());
        directionalShadowPipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE; // TODO: Why is this clockwise??
        directionalShadowPipelineDesc.viewport = CustomViewport( DIRECTIONAL_SHADOW_MAP_RESOLUTION, DIRECTIONAL_SHADOW_MAP_RESOLUTION );
        directionalShadowPipelineDesc.scissor = FullScreenScissor();
        directionalShadowPipelineDesc.shaders[0] = directionalShadowVert.get();
        directionalShadowPipelineDesc.numShaders = 1;

        directionalShadow.pipeline = g_renderState.device.NewPipeline(directionalShadowPipelineDesc);
        if (!directionalShadow.pipeline)
        {
            LOG_ERR("Could not create directional shadow pipeline");
            return false;
        }


        
        info.type = ImageType::TYPE_2D;
        info.format = PixelFormat::DEPTH_32_FLOAT;
        info.width = DIRECTIONAL_SHADOW_MAP_RESOLUTION;
        info.height = DIRECTIONAL_SHADOW_MAP_RESOLUTION;
        info.sampler = "nearest_clamped_nearest";
        info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        directionalShadow.depthAttachment = g_renderState.device.NewTexture(info, true);


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

        // Free Post Processing Data
        offScreenRenderData.colorAttachment.Free();
        offScreenRenderData.renderPass.Free();
        vkDestroyFramebuffer(g_renderState.device.GetHandle(), offScreenRenderData.frameBuffer, nullptr);
        offScreenRenderData.postPorcessingPipeline.Free();
        for (auto& layout : offScreenRenderData.textureToProcessLayout)
        {
            layout.Free();
        }
        offScreenRenderData.quadBuffer.Free();
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

        // Post Processing Render Pass
        cmdBuf.BeginRenderPass( g_renderState.renderPass, g_renderState.swapChainFramebuffers[imageIndex] );

        cmdBuf.BindRenderPipeline( offScreenRenderData.postPorcessingPipeline );
        cmdBuf.BindDescriptorSets( 1, &offScreenRenderData.textureToProcess, offScreenRenderData.postPorcessingPipeline, 0 );
        
        cmdBuf.BindVertexBuffer( offScreenRenderData.quadBuffer, 0, 0 );
        cmdBuf.Draw( 0, 6 );

        cmdBuf.EndRenderPass();

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
