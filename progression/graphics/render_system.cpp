#include "graphics/render_system.hpp"
#include "core/animation_system.hpp"
#include "core/assert.hpp"
#include "core/scene.hpp"
#include "core/time.hpp"
#include "core/window.hpp"
#include "components/model_renderer.hpp"
#include "components/script_component.hpp"
#include "components/skinned_renderer.hpp"
#include "graphics/debug_marker.hpp"
#include "graphics/graphics_api.hpp"
#include "graphics/pg_to_vulkan_types.hpp"
#include "graphics/shader_c_shared/defines.h"
#include "graphics/shader_c_shared/structs.h"
#include "graphics/texture_manager.hpp"
#include "graphics/vulkan.hpp"
#include "resource/resource_manager.hpp"
#include "resource/image.hpp"
#include "resource/model.hpp"
#include "resource/shader.hpp"
#include "utils/logger.hpp"
#include <array>
#include <unordered_map>

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

struct GBuffer
{
    Gfx::Texture positions;
    Gfx::Texture normals;
    Gfx::Texture diffuse;
    Gfx::Texture specular;
};

struct GBufferRenderData
{
    Gfx::RenderPass renderPass;
    GBuffer gbuffer;
    VkFramebuffer frameBuffer;
    Gfx::Pipeline pipeline;
    Gfx::DescriptorSet descriptorSet;
    std::vector< Gfx::DescriptorSetLayout > descriptorSetLayout;
} gBufferRenderData;

struct PostProcessRenderData
{
    Gfx::RenderPass renderPass;
    Gfx::Texture colorAttachment;
    VkFramebuffer frameBuffer;
	Gfx::Pipeline postPorcessingPipeline;
    Gfx::Buffer quadBuffer;
    Gfx::DescriptorSet textureToProcess;
    std::vector< Gfx::DescriptorSetLayout > textureToProcessLayout;
} postProcessRenderData;

struct ShadowRenderData
{
    Gfx::RenderPass renderPass;
    Gfx::Texture    depthAttachment;
    VkFramebuffer   frameBuffer;
    Gfx::Pipeline   pipeline;
    glm::mat4       LSM;
} directionalShadow;

#define MAX_NUM_POINT_LIGHTS 1024
#define MAX_NUM_SPOT_LIGHTS 256
#define DIRECTIONAL_SHADOW_MAP_RESOLUTION 4096

static bool InitShadowPassData()
{
    RenderPassDescriptor desc;
    desc.depthAttachmentDescriptor.format      = PixelFormat::DEPTH_32_FLOAT;
    desc.depthAttachmentDescriptor.loadAction  = LoadAction::CLEAR;
    desc.depthAttachmentDescriptor.storeAction = StoreAction::STORE;
    desc.depthAttachmentDescriptor.layout      = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    
    directionalShadow.renderPass = g_renderState.device.NewRenderPass( desc, "directionalShadow" );
    if ( !directionalShadow.renderPass )
    {
        return false;
    }

    auto directionalShadowVert = ResourceManager::Get< Shader >( "directionalShadowVert" );

    VertexBindingDescriptor directionalShadowBindingDesc[1];
    directionalShadowBindingDesc[0].binding   = 0;
    directionalShadowBindingDesc[0].stride    = sizeof(glm::vec3);
    directionalShadowBindingDesc[0].inputRate = VertexInputRate::PER_VERTEX;

    std::array< VertexAttributeDescriptor, 1 > directionalShadowAttribDescs;
    directionalShadowAttribDescs[0].binding  = 0;
    directionalShadowAttribDescs[0].location = 0;
    directionalShadowAttribDescs[0].format   = BufferDataType::FLOAT3;
    directionalShadowAttribDescs[0].offset   = 0;

    PipelineDescriptor directionalShadowPipelineDesc;
    directionalShadowPipelineDesc.rasterizerInfo.depthBiasEnable = true;
    directionalShadowPipelineDesc.renderPass       = &directionalShadow.renderPass;
    directionalShadowPipelineDesc.vertexDescriptor = VertexInputDescriptor::Create( 1, directionalShadowBindingDesc, 1, directionalShadowAttribDescs.data() );
    directionalShadowPipelineDesc.viewport         = Viewport( DIRECTIONAL_SHADOW_MAP_RESOLUTION, -DIRECTIONAL_SHADOW_MAP_RESOLUTION );
    directionalShadowPipelineDesc.viewport.y       = DIRECTIONAL_SHADOW_MAP_RESOLUTION;
    Scissor scissor                                = {};
    scissor.width                                  = DIRECTIONAL_SHADOW_MAP_RESOLUTION;
    scissor.height                                 = DIRECTIONAL_SHADOW_MAP_RESOLUTION;
    directionalShadowPipelineDesc.scissor          = scissor;
    directionalShadowPipelineDesc.shaders[0]       = directionalShadowVert.get();
    directionalShadowPipelineDesc.numShaders       = 1;
    directionalShadowPipelineDesc.numColorAttachments = 0;

    directionalShadow.pipeline = g_renderState.device.NewPipeline( directionalShadowPipelineDesc, "directional shadow pass" );
    if ( !directionalShadow.pipeline )
    {
        LOG_ERR( "Could not create directional shadow pipeline" );
        return false;
    }
 
    ImageDescriptor info;
    info.type    = ImageType::TYPE_2D;
    info.format  = PixelFormat::DEPTH_32_FLOAT;
    info.width   = DIRECTIONAL_SHADOW_MAP_RESOLUTION;
    info.height  = DIRECTIONAL_SHADOW_MAP_RESOLUTION;
    info.sampler = "shadow_map";
    info.usage   = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    directionalShadow.depthAttachment = g_renderState.device.NewTexture( info, true, "directional shadowmap" );

    VkImageView frameBufferAttachments[1];
    frameBufferAttachments[0] = directionalShadow.depthAttachment.GetView();

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = directionalShadow.renderPass.GetHandle();
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments    = frameBufferAttachments;
    framebufferInfo.width           = DIRECTIONAL_SHADOW_MAP_RESOLUTION;
    framebufferInfo.height          = DIRECTIONAL_SHADOW_MAP_RESOLUTION;
    framebufferInfo.layers          = 1;

    if ( vkCreateFramebuffer( g_renderState.device.GetHandle(), &framebufferInfo, nullptr, &directionalShadow.frameBuffer ) != VK_SUCCESS )
    {
        LOG_ERR( "Could not create shadow framebuffer" );
        return false;
    }
    PG_DEBUG_MARKER_SET_FRAMEBUFFER_NAME( directionalShadow.frameBuffer, "directional shadow pass" );

    return true;
}


bool InitPostProcessRenderPass()
{
    RenderPassDescriptor desc;
    desc.colorAttachmentDescriptors[0].format     = VulkanToPGPixelFormat( g_renderState.swapChain.imageFormat );
    desc.colorAttachmentDescriptors[0].clearColor = glm::vec4( .2, .2, .2, 1 );
    desc.colorAttachmentDescriptors[0].layout     = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    desc.depthAttachmentDescriptor.format         = PixelFormat::DEPTH_32_FLOAT;
    desc.depthAttachmentDescriptor.loadAction     = LoadAction::CLEAR;
    desc.depthAttachmentDescriptor.storeAction    = StoreAction::STORE;
    desc.depthAttachmentDescriptor.layout         = ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    postProcessRenderData.renderPass = g_renderState.device.NewRenderPass( desc, "postProcess" );
    if ( !postProcessRenderData.renderPass )
    {
        return false;
    }

    return true;
}

static bool InitGBufferPassData()
{
    RenderPassDescriptor desc;
    desc.colorAttachmentDescriptors[0].format  = PixelFormat::R32_G32_B32_A32_FLOAT;
    desc.colorAttachmentDescriptors[0].layout  = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    desc.colorAttachmentDescriptors[1].format  = PixelFormat::R32_G32_B32_A32_FLOAT;
    desc.colorAttachmentDescriptors[1].layout  = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    desc.colorAttachmentDescriptors[2].format  = PixelFormat::R8_G8_B8_A8_UNORM;
    desc.colorAttachmentDescriptors[2].layout  = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    desc.colorAttachmentDescriptors[3].format  = PixelFormat::R16_G16_B16_A16_FLOAT; //TODO!!! Reduce number of textures by packing both diffuse and specular into rgb and the specular exponent in a
    desc.colorAttachmentDescriptors[3].layout  = ImageLayout::SHADER_READ_ONLY_OPTIMAL;

    desc.depthAttachmentDescriptor.format      = PixelFormat::DEPTH_32_FLOAT;
    desc.depthAttachmentDescriptor.loadAction  = LoadAction::CLEAR;
    desc.depthAttachmentDescriptor.storeAction = StoreAction::STORE;
    desc.depthAttachmentDescriptor.layout      = ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    gBufferRenderData.renderPass = g_renderState.device.NewRenderPass( desc, "gBuffer" );
    if ( !gBufferRenderData.renderPass )
    {
        return false;
    }

    VertexBindingDescriptor bindingDesc[4];
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
    bindingDesc[3].stride    = sizeof( glm::vec3 );
    bindingDesc[3].inputRate = VertexInputRate::PER_VERTEX;

    VertexAttributeDescriptor attribDescs[4];
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
    attribDescs[3].format   = BufferDataType::FLOAT3;
    attribDescs[3].offset   = 0;

    auto rigidModelsVert		= ResourceManager::Get< Shader >( "rigidModelsVert" );
    auto gBufferFrag            = ResourceManager::Get< Shader >( "gBufferFrag" );

    PipelineDescriptor pipelineDesc;
    pipelineDesc.renderPass             = &gBufferRenderData.renderPass;
    pipelineDesc.descriptorSetLayouts   = s_descriptorSetLayouts; //gBufferRenderData.descriptorSetLayout
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 4, bindingDesc, 4, attribDescs );
    pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.viewport               = FullScreenViewport();
    pipelineDesc.viewport.height        = -pipelineDesc.viewport.height;
    pipelineDesc.viewport.y             = -pipelineDesc.viewport.height;
    pipelineDesc.scissor                = FullScreenScissor();
    pipelineDesc.shaders[0]             = rigidModelsVert.get();
    pipelineDesc.shaders[1]             = gBufferFrag.get(); //forwardBlinnPhongFrag.get();
    pipelineDesc.numShaders             = 2;
    pipelineDesc.numColorAttachments    = 4;

    gBufferRenderData.pipeline = g_renderState.device.NewPipeline( pipelineDesc, "gbuffer rigid model" );
    if ( !gBufferRenderData.pipeline )
    {
        LOG_ERR( "Could not create gbuffer pipeline" );
        return false;
    }

    ImageDescriptor info;
    info.type    = ImageType::TYPE_2D;
    info.width   = g_renderState.swapChain.extent.width;
    info.height  = g_renderState.swapChain.extent.height;
    info.sampler = "nearest_clamped_nearest";
    info.usage   = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    info.format                         = PixelFormat::R32_G32_B32_A32_FLOAT;
    gBufferRenderData.gbuffer.positions = g_renderState.device.NewTexture( info, false, "gbuffer position" );
    info.format                         = PixelFormat::R32_G32_B32_A32_FLOAT;
    gBufferRenderData.gbuffer.normals   = g_renderState.device.NewTexture( info, false, "gbuffer normals" );
    info.format                         = PixelFormat::R8_G8_B8_A8_UNORM;
    gBufferRenderData.gbuffer.diffuse   = g_renderState.device.NewTexture( info, false, "gbuffer diffuse" );
    info.format                         = PixelFormat::R16_G16_B16_A16_FLOAT;
    gBufferRenderData.gbuffer.specular  = g_renderState.device.NewTexture( info, false, "gbuffer specular" );

    VkImageView attachments[5];
    attachments[0] = gBufferRenderData.gbuffer.positions.GetView();
    attachments[1] = gBufferRenderData.gbuffer.normals.GetView();
    attachments[2] = gBufferRenderData.gbuffer.diffuse.GetView();
    attachments[3] = gBufferRenderData.gbuffer.specular.GetView();
    attachments[4] = g_renderState.depthTex.GetView();
   
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = gBufferRenderData.renderPass.GetHandle();
    framebufferInfo.attachmentCount = 5;
    framebufferInfo.pAttachments    = attachments;
    framebufferInfo.width           = g_renderState.swapChain.extent.width;
    framebufferInfo.height          = g_renderState.swapChain.extent.height;
    framebufferInfo.layers          = 1;

    if ( vkCreateFramebuffer( g_renderState.device.GetHandle(), &framebufferInfo, nullptr, &gBufferRenderData.frameBuffer ) != VK_SUCCESS )
    {
        LOG_ERR( "Could not create gbuffer framebuffer" );
        return false;
    }
    PG_DEBUG_MARKER_SET_FRAMEBUFFER_NAME( gBufferRenderData.frameBuffer, "gbuffer pass" );

    return true;
}


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
                    BUFFER_TYPE_UNIFORM, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT, "Scene Constants " + std::to_string( i ) );
            s_gpuPointLightBuffers[i] = g_renderState.device.NewBuffer( sizeof( PointLight ) * MAX_NUM_POINT_LIGHTS,
                    BUFFER_TYPE_STORAGE, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT, "Point Lights " + std::to_string( i ) );
            s_gpuSpotLightBuffers[i] = g_renderState.device.NewBuffer( sizeof( SpotLight ) * MAX_NUM_SPOT_LIGHTS,
                    BUFFER_TYPE_STORAGE, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT, "Spot Lights " + std::to_string( i ) );
        }

        VkDescriptorPoolSize poolSize[3] = {};
        poolSize[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize[0].descriptorCount = numImages;
        poolSize[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSize[1].descriptorCount = 3 * numImages;
        poolSize[2].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize[2].descriptorCount = numImages + 1;

        s_descriptorPool = g_renderState.device.NewDescriptorPool( 3, poolSize, 3 * numImages, "render system" );
                
		// Post Processing
        if ( !InitPostProcessRenderPass() )
        {
            return false;
        }
        
        auto rigidModelsVert		= ResourceManager::Get< Shader >( "rigidModelsVert" );
        auto forwardBlinnPhongFrag  = ResourceManager::Get< Shader >( "forwardBlinnPhongFrag" );

        std::vector< DescriptorSetLayoutData > descriptorSetData = rigidModelsVert->reflectInfo.descriptorSetLayouts;
        descriptorSetData.insert( descriptorSetData.end(), forwardBlinnPhongFrag->reflectInfo.descriptorSetLayouts.begin(), forwardBlinnPhongFrag->reflectInfo.descriptorSetLayouts.end() );
        auto combined = CombineDescriptorSetLayouts( descriptorSetData );

        s_descriptorSetLayouts = g_renderState.device.NewDescriptorSetLayouts( combined );
        sceneDescriptorSets    = s_descriptorPool.NewDescriptorSets( numImages, s_descriptorSetLayouts[0], "scene data" );
        textureDescriptorSets  = s_descriptorPool.NewDescriptorSets( numImages, s_descriptorSetLayouts[1], "textures" );

        if ( !InitGBufferPassData() )
        {
            LOG_ERR( "Could not init gbuffer pass data" );
            return false;
        }

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
 
        VertexBindingDescriptor bindingDesc[4];
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
        bindingDesc[3].stride    = sizeof( glm::vec3 );
        bindingDesc[3].inputRate = VertexInputRate::PER_VERTEX;

        VertexAttributeDescriptor attribDescs[4];
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
        attribDescs[3].format   = BufferDataType::FLOAT3;
        attribDescs[3].offset   = 0;

        PipelineDescriptor pipelineDesc;
        pipelineDesc.renderPass             = &postProcessRenderData.renderPass;
        pipelineDesc.descriptorSetLayouts   = s_descriptorSetLayouts;
        pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 4, bindingDesc, 4, attribDescs );
        pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
        pipelineDesc.viewport               = FullScreenViewport();
        pipelineDesc.viewport.height        = -pipelineDesc.viewport.height;
        pipelineDesc.viewport.y             = -pipelineDesc.viewport.height;
        pipelineDesc.scissor                = FullScreenScissor();
        pipelineDesc.shaders[0]             = rigidModelsVert.get();
        pipelineDesc.shaders[1]             = forwardBlinnPhongFrag.get();
        pipelineDesc.numShaders             = 2;
        pipelineDesc.numColorAttachments    = 1;

        s_rigidModelPipeline = g_renderState.device.NewPipeline( pipelineDesc, "rigid model" );
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
        postProcessRenderData.colorAttachment = g_renderState.device.NewTexture( info, false, "offscreen color attachment" );

        VkImageView attachments[2];
        attachments[0] = postProcessRenderData.colorAttachment.GetView();
        attachments[1] = g_renderState.depthTex.GetView();

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = postProcessRenderData.renderPass.GetHandle();
        framebufferInfo.attachmentCount = 2;
        framebufferInfo.pAttachments    = attachments;
        framebufferInfo.width           = postProcessRenderData.colorAttachment.GetWidth();
        framebufferInfo.height          = postProcessRenderData.colorAttachment.GetHeight();
        framebufferInfo.layers          = 1;

        if ( vkCreateFramebuffer( g_renderState.device.GetHandle(), &framebufferInfo, nullptr, &postProcessRenderData.frameBuffer ) != VK_SUCCESS )
        {
            LOG_ERR( "Could not create offscreen framebuffer" );
            return false;
        }
        PG_DEBUG_MARKER_SET_FRAMEBUFFER_NAME( postProcessRenderData.frameBuffer, "offscreen pass (gbuffer)" );

		// Post Processing

        // Post Processing shaders
        auto postProcessingVert = ResourceManager::Get< Shader >( "postProcessVert" );
        auto postProcessingFrag = ResourceManager::Get< Shader >( "postProcessFrag" );

        descriptorSetData = postProcessingVert->reflectInfo.descriptorSetLayouts;
        descriptorSetData.insert( descriptorSetData.end(), postProcessingFrag->reflectInfo.descriptorSetLayouts.begin(), postProcessingFrag->reflectInfo.descriptorSetLayouts.end() );
        combined = CombineDescriptorSetLayouts( descriptorSetData );

        postProcessRenderData.textureToProcessLayout = g_renderState.device.NewDescriptorSetLayouts( combined );
        postProcessRenderData.textureToProcess = s_descriptorPool.NewDescriptorSets( 1, postProcessRenderData.textureToProcessLayout[0], "post process" )[0];

		VertexBindingDescriptor postProcescsingBindingDesc[1];
        postProcescsingBindingDesc[0].binding = 0;
        postProcescsingBindingDesc[0].stride = sizeof( glm::vec3 );
        postProcescsingBindingDesc[0].inputRate = VertexInputRate::PER_VERTEX;

		std::array< VertexAttributeDescriptor, 1 > postProcescsingAttribDescs;
        postProcescsingAttribDescs[0].binding = 0;
        postProcescsingAttribDescs[0].location = 0;
        postProcescsingAttribDescs[0].format = BufferDataType::FLOAT3;
        postProcescsingAttribDescs[0].offset = 0;

		PipelineDescriptor postProcessingPipelineDesc;
        postProcessingPipelineDesc.renderPass	          = &g_renderState.renderPass;
        postProcessingPipelineDesc.descriptorSetLayouts   = postProcessRenderData.textureToProcessLayout;
        postProcessingPipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 1, postProcescsingBindingDesc, 1, postProcescsingAttribDescs.data() );
        postProcessingPipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
        postProcessingPipelineDesc.viewport               = FullScreenViewport();
        postProcessingPipelineDesc.scissor                = FullScreenScissor();
        postProcessingPipelineDesc.shaders[0]	          = postProcessingVert.get();
        postProcessingPipelineDesc.shaders[1]			  = postProcessingFrag.get();
        postProcessingPipelineDesc.numShaders             = 2;
        postProcessingPipelineDesc.numColorAttachments    = 1;

		postProcessRenderData.postPorcessingPipeline = g_renderState.device.NewPipeline( postProcessingPipelineDesc , "post process");
		if ( !postProcessRenderData.postPorcessingPipeline )
		{
			LOG_ERR( "Could not create post processing pipeline" );
			return false;
		}

        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.sampler = postProcessRenderData.colorAttachment.GetSampler()->GetHandle();
        imageInfo.imageView = postProcessRenderData.colorAttachment.GetView();

        VkWriteDescriptorSet descriptorWrite[1] = {};
        descriptorWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite[0].dstSet = postProcessRenderData.textureToProcess.GetHandle();
        descriptorWrite[0].dstBinding = 0;
        descriptorWrite[0].dstArrayElement = 0;
        descriptorWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite[0].descriptorCount = 1;
        descriptorWrite[0].pImageInfo = &imageInfo;

        g_renderState.device.UpdateDescriptorSets( 1, descriptorWrite );

        glm::vec3 quadVerts[] =
        {
            glm::vec3( -1, -1, 0 ),
            glm::vec3( -1,  1, 0 ),
            glm::vec3(  1,  1, 0 ),
            glm::vec3( -1, -1, 0 ),
            glm::vec3(  1,  1, 0 ),
            glm::vec3(  1, -1, 0 ),
        };

        postProcessRenderData.quadBuffer = g_renderState.device.NewBuffer( sizeof( quadVerts ), quadVerts, BUFFER_TYPE_VERTEX, MEMORY_TYPE_DEVICE_LOCAL, "RenderSystem Quad VBO" );

        if ( !InitShadowPassData() )
        {
            LOG_ERR( "Could not init shadow pass data" );
            return false;
        }

        return true;
    }

    void Shutdown()
    {
        g_renderState.device.WaitForIdle();

        directionalShadow.depthAttachment.Free();
        directionalShadow.renderPass.Free();
        directionalShadow.pipeline.Free();
        vkDestroyFramebuffer( g_renderState.device.GetHandle(), directionalShadow.frameBuffer, nullptr );

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
        postProcessRenderData.colorAttachment.Free();
        postProcessRenderData.renderPass.Free();
        vkDestroyFramebuffer( g_renderState.device.GetHandle(), postProcessRenderData.frameBuffer, nullptr );
        postProcessRenderData.postPorcessingPipeline.Free();
        for ( auto& layout : postProcessRenderData.textureToProcessLayout )
        {
            layout.Free();
        }
        postProcessRenderData.quadBuffer.Free();

        // Free GBuffer Data
        gBufferRenderData.gbuffer.positions.Free();
        gBufferRenderData.gbuffer.normals.Free();
        gBufferRenderData.gbuffer.diffuse.Free();
        gBufferRenderData.gbuffer.specular.Free();
        gBufferRenderData.renderPass.Free();
        vkDestroyFramebuffer( g_renderState.device.GetHandle(), gBufferRenderData.frameBuffer, nullptr );
        gBufferRenderData.pipeline.Free();
        for ( auto& layout : gBufferRenderData.descriptorSetLayout)
        {
            layout.Free();
        }
    }

    void ShadowPass( Scene* scene, CommandBuffer& cmdBuf )
    {
        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "Shadow Pass", glm::vec4( .2, .2, .4, 1 ) );
        // cmdBuf.BeginRenderPass( directionalShadow.renderPass, directionalShadow.frameBuffer );
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass        = directionalShadow.renderPass.GetHandle();
        renderPassInfo.framebuffer       = directionalShadow.frameBuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = { DIRECTIONAL_SHADOW_MAP_RESOLUTION, DIRECTIONAL_SHADOW_MAP_RESOLUTION };

        VkClearValue clearValues[1] = {};
        clearValues[0].depthStencil = { 1.0f, 0 };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues    = clearValues;
        vkCmdBeginRenderPass( cmdBuf.GetHandle(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "Shadow rigid models", glm::vec4( .2, .6, .4, 1 ) );
        cmdBuf.BindRenderPipeline( directionalShadow.pipeline );
        cmdBuf.SetDepthBias( 2, 0, 9.5 ); // Values that 'worked' (removed many artifacts) epirically for sponza.json
        scene->registry.view< ModelRenderer, Transform >().each( [&]( ModelRenderer& modelRenderer, Transform& transform )
        {
            const auto& model = modelRenderer.model;
            auto MVP = directionalShadow.LSM * transform.GetModelMatrix();
            vkCmdPushConstants( cmdBuf.GetHandle(), directionalShadow.pipeline.GetLayoutHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( glm::mat4 ), &MVP[0][0] );

            cmdBuf.BindVertexBuffer( model->vertexBuffer, 0, 0 );
            cmdBuf.BindIndexBuffer(  model->indexBuffer, model->GetIndexType() );

            for ( size_t i = 0; i < model->meshes.size(); ++i )
            {
                const auto& mesh = modelRenderer.model->meshes[i];
                PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw \"" + model->name + "\" : \"" + mesh.name + "\"", glm::vec4( 0 ) );
                cmdBuf.DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
            }
        });
        PG_DEBUG_MARKER_END_REGION( cmdBuf );

        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "Shadow animated models", glm::vec4( .6, .2, .4, 1 ) );
        auto& animPipeline = AnimationSystem::renderData.animatedPipeline;
        scene->registry.view< Animator, SkinnedRenderer, Transform >().each( [&]( Animator& animator, SkinnedRenderer& renderer, Transform& transform )
        {
            const auto& model = renderer.model;
            auto MVP          = directionalShadow.LSM * transform.GetModelMatrix();
            vkCmdPushConstants( cmdBuf.GetHandle(), animPipeline.GetLayoutHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( glm::mat4 ), &MVP[0][0] );

            cmdBuf.BindVertexBuffer( model->vertexBuffer, 0, 0 );
            cmdBuf.BindIndexBuffer(  model->indexBuffer, model->GetIndexType() );

            for ( size_t i = 0; i < renderer.model->meshes.size(); ++i )
            {
                const auto& mesh = model->meshes[i];
                PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw \"" + model->name + "\" : \"" + mesh.name + "\"", glm::vec4( 0 ) );
                cmdBuf.DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
            }
        });
        PG_DEBUG_MARKER_END_REGION( cmdBuf );

        cmdBuf.EndRenderPass();
        PG_DEBUG_MARKER_END_REGION( cmdBuf );
    }

    void Render( Scene* scene )
    {
        PG_ASSERT( scene != nullptr );
        PG_ASSERT( scene->pointLights.size() < MAX_NUM_POINT_LIGHTS && scene->spotLights.size() < MAX_NUM_SPOT_LIGHTS );
        size_t currentFrame = g_renderState.currentFrame;
        g_renderState.inFlightFences[currentFrame].WaitFor();
        g_renderState.inFlightFences[currentFrame].Reset();

        auto imageIndex = g_renderState.swapChain.AcquireNextImage( g_renderState.presentCompleteSemaphores[currentFrame] );

        TextureManager::UpdateDescriptors( textureDescriptorSets );

        // TODO: Remove hardcoded directional shadow map
        glm::vec3 pos( 0, 15, 0 );
        auto V = glm::lookAt( pos, pos + glm::vec3( scene->directionalLight.direction ), glm::vec3( 0, 1, 0 ) );
        float W = 25;
        auto P = glm::ortho( -W, W, -W, W, 0.0f, 50.0f );
        directionalShadow.LSM = P * V;

        // sceneConstantBuffer
        SceneConstantBufferData scbuf;
        scbuf.V              = scene->camera.GetV();
        scbuf.P              = scene->camera.GetP();
        scbuf.VP             = scene->camera.GetVP();
        scbuf.DLSM           = directionalShadow.LSM;
        scbuf.cameraPos      = glm::vec4( scene->camera.position, 0 );        
        scbuf.ambientColor   = glm::vec4( scene->ambientColor, 0 );
        scbuf.dirLight       = scene->directionalLight;
        scbuf.numPointLights = static_cast< uint32_t >( scene->pointLights.size() );
        scbuf.numSpotLights  = static_cast< uint32_t >( scene->spotLights.size() );
        scbuf.shadowTextureIndex = directionalShadow.depthAttachment.GetShaderSlot();
        s_gpuSceneConstantBuffers[imageIndex].Map();
        memcpy( s_gpuSceneConstantBuffers[imageIndex].MappedPtr(), &scbuf, sizeof( SceneConstantBufferData ) );
        s_gpuSceneConstantBuffers[imageIndex].UnMap();

        s_gpuPointLightBuffers[imageIndex].Map();
        memcpy( s_gpuPointLightBuffers[imageIndex].MappedPtr(), scene->pointLights.data(), scene->pointLights.size() * sizeof( PointLight ) );
        s_gpuPointLightBuffers[imageIndex].UnMap();

        s_gpuSpotLightBuffers[imageIndex].Map();
        memcpy( s_gpuSpotLightBuffers[imageIndex].MappedPtr(), scene->spotLights.data(), scene->spotLights.size() * sizeof( SpotLight ) );
        s_gpuSpotLightBuffers[imageIndex].UnMap();

        AnimationSystem::UploadToGpu( scene, imageIndex );

        auto& cmdBuf = g_renderState.commandBuffers[imageIndex];
        cmdBuf.BeginRecording();

        ShadowPass( scene, cmdBuf );

        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "GBuffer Pass", glm::vec4( .8, .8, .2, 1 ) );
        cmdBuf.BeginRenderPass( gBufferRenderData.renderPass, gBufferRenderData.frameBuffer );
        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "GBuffer -- Rigid Models", glm::vec4( .2, .8, .2, 1 ) );
        cmdBuf.BindRenderPipeline( gBufferRenderData.pipeline );
        cmdBuf.BindDescriptorSets( 1, &sceneDescriptorSets[imageIndex], gBufferRenderData.pipeline, PG_SCENE_CONSTANT_BUFFER_SET );
        cmdBuf.BindDescriptorSets( 1, &textureDescriptorSets[imageIndex], gBufferRenderData.pipeline, PG_2D_TEXTURES_SET );

        scene->registry.view< ModelRenderer, Transform >().each( [&]( ModelRenderer& modelRenderer, Transform& transform )
        {
            const auto& model = modelRenderer.model;
            // TODO: Actually fix this for models without tangets as well
            if ( model->GetTangentOffset() == ~0u )
            {
                return;
            }
            
            auto M = transform.GetModelMatrix();
            auto N = glm::transpose( glm::inverse( M ) );
            ObjectConstantBufferData b;
            b.M = M;
            b.N = N;
            vkCmdPushConstants( cmdBuf.GetHandle(), gBufferRenderData.pipeline.GetLayoutHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( ObjectConstantBufferData ), &b );

            cmdBuf.BindVertexBuffer( model->vertexBuffer, 0, 0 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetNormalOffset(), 1 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetUVOffset(), 2 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetTangentOffset(), 3 );
            cmdBuf.BindIndexBuffer(  model->indexBuffer, model->GetIndexType() );

            for ( size_t i = 0; i < model->meshes.size(); ++i )
            {
                const auto& mesh = modelRenderer.model->meshes[i];
                const auto& mat  = modelRenderer.materials[mesh.materialIndex];

                MaterialConstantBufferData mcbuf{};
                mcbuf.Kd = glm::vec4( mat->Kd, 0 );
                mcbuf.Ks = glm::vec4( mat->Ks, mat->Ns );
                mcbuf.diffuseTexIndex = mat->map_Kd   ? mat->map_Kd->GetTexture()->GetShaderSlot()   : PG_INVALID_TEXTURE_INDEX;
                mcbuf.normalMapIndex  = mat->map_Norm ? mat->map_Norm->GetTexture()->GetShaderSlot() : PG_INVALID_TEXTURE_INDEX;
                vkCmdPushConstants( cmdBuf.GetHandle(), gBufferRenderData.pipeline.GetLayoutHandle(), VK_SHADER_STAGE_FRAGMENT_BIT, PG_MATERIAL_PUSH_CONSTANT_OFFSET, sizeof( MaterialConstantBufferData ), &mcbuf );

                PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw \"" + model->name + "\" : \"" + mesh.name + "\"", glm::vec4( 0 ) );
                cmdBuf.DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
            }
        });
        PG_DEBUG_MARKER_END_REGION( cmdBuf );
        cmdBuf.EndRenderPass();

        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "GBuffer Pass", glm::vec4( .8, .8, .2, 1 ) );
        cmdBuf.BeginRenderPass( postProcessRenderData.renderPass, postProcessRenderData.frameBuffer );
        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "GBuffer -- Rigid Models", glm::vec4( .2, .8, .2, 1 ) );
        cmdBuf.BindRenderPipeline( s_rigidModelPipeline );
        cmdBuf.BindDescriptorSets( 1, &sceneDescriptorSets[imageIndex], s_rigidModelPipeline, PG_SCENE_CONSTANT_BUFFER_SET );
        cmdBuf.BindDescriptorSets( 1, &textureDescriptorSets[imageIndex], s_rigidModelPipeline, PG_2D_TEXTURES_SET );

        scene->registry.view< ModelRenderer, Transform >().each( [&]( ModelRenderer& modelRenderer, Transform& transform )
        {
            const auto& model = modelRenderer.model;
            // TODO: Actually fix this for models without tangets as well
            if ( model->GetTangentOffset() == ~0u )
            {
                return;
            }
            
            auto M = transform.GetModelMatrix();
            auto N = glm::transpose( glm::inverse( M ) );
            ObjectConstantBufferData b;
            b.M = M;
            b.N = N;
            vkCmdPushConstants( cmdBuf.GetHandle(), s_rigidModelPipeline.GetLayoutHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( ObjectConstantBufferData ), &b );

            cmdBuf.BindVertexBuffer( model->vertexBuffer, 0, 0 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetNormalOffset(), 1 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetUVOffset(), 2 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetTangentOffset(), 3 );
            cmdBuf.BindIndexBuffer(  model->indexBuffer, model->GetIndexType() );

            for ( size_t i = 0; i < model->meshes.size(); ++i )
            {
                const auto& mesh = modelRenderer.model->meshes[i];
                const auto& mat  = modelRenderer.materials[mesh.materialIndex];

                MaterialConstantBufferData mcbuf{};
                mcbuf.Kd = glm::vec4( mat->Kd, 0 );
                mcbuf.Ks = glm::vec4( mat->Ks, mat->Ns );
                mcbuf.diffuseTexIndex = mat->map_Kd   ? mat->map_Kd->GetTexture()->GetShaderSlot()   : PG_INVALID_TEXTURE_INDEX;
                mcbuf.normalMapIndex  = mat->map_Norm ? mat->map_Norm->GetTexture()->GetShaderSlot() : PG_INVALID_TEXTURE_INDEX;
                vkCmdPushConstants( cmdBuf.GetHandle(), s_rigidModelPipeline.GetLayoutHandle(), VK_SHADER_STAGE_FRAGMENT_BIT, PG_MATERIAL_PUSH_CONSTANT_OFFSET, sizeof( MaterialConstantBufferData ), &mcbuf );

                PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw \"" + model->name + "\" : \"" + mesh.name + "\"", glm::vec4( 0 ) );
                cmdBuf.DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
            }
        });
        PG_DEBUG_MARKER_END_REGION( cmdBuf );

        /*
        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "Render Animated Models", glm::vec4( .8, .2, .2, 1 ) );
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
                mcbuf.Kd = glm::vec4( mat->Kd, 0 );
                mcbuf.Ks = glm::vec4( mat->Ks, mat->Ns );
                mcbuf.diffuseTexIndex = mat->map_Kd ? mat->map_Kd->GetTexture()->GetShaderSlot() : PG_INVALID_TEXTURE_INDEX;
                mcbuf.normalMapIndex  = mat->map_Norm ? mat->map_Norm->GetTexture()->GetShaderSlot() : PG_INVALID_TEXTURE_INDEX;
                vkCmdPushConstants( cmdBuf.GetHandle(), animPipeline.GetLayoutHandle(), VK_SHADER_STAGE_FRAGMENT_BIT, PG_MATERIAL_PUSH_CONSTANT_OFFSET, sizeof( MaterialConstantBufferData ), &mcbuf );
                
                PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw \"" + model->name + "\" : \"" + mesh.name + "\"", glm::vec4( 0 ) );
                cmdBuf.DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
            }
        });

        PG_DEBUG_MARKER_END_REGION( cmdBuf );
        */
        cmdBuf.EndRenderPass();

        PG_DEBUG_MARKER_END_REGION( cmdBuf );

        // Post Processing Render Pass
        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "Post Process", glm::vec4( .2, .2, 1, 1 ) );
        cmdBuf.BeginRenderPass( g_renderState.renderPass, g_renderState.swapChainFramebuffers[imageIndex] );
        
        cmdBuf.BindRenderPipeline( postProcessRenderData.postPorcessingPipeline );
        cmdBuf.BindDescriptorSets( 1, &postProcessRenderData.textureToProcess, postProcessRenderData.postPorcessingPipeline, 0 );
        
        PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw full-screen quad", glm::vec4( 0 ) );
        cmdBuf.BindVertexBuffer( postProcessRenderData.quadBuffer, 0, 0 );
        cmdBuf.Draw( 0, 6 );
        
        cmdBuf.EndRenderPass();
        PG_DEBUG_MARKER_END_REGION( cmdBuf );

        cmdBuf.EndRecording();
        g_renderState.device.SubmitRenderCommands( 1, &cmdBuf );
        // g_renderState.computeFence.WaitFor();
        // g_renderState.computeFence.Reset();
        // g_renderState.device.SubmitComputeCommand( g_renderState.computeCommandBuffer );
        g_renderState.device.SubmitFrame( imageIndex );
    } 

    void InitSamplers()
    {
        SamplerDescriptor samplerDesc;

        samplerDesc.name      = "nearest_clamped_nearest";
        samplerDesc.minFilter = FilterMode::NEAREST;
        samplerDesc.magFilter = FilterMode::NEAREST;
        samplerDesc.mipFilter = MipFilterMode::NEAREST;
        samplerDesc.wrapModeU = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeV = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeW = WrapMode::CLAMP_TO_EDGE;
        AddSampler( samplerDesc );

        samplerDesc.name        = "shadow_map";
        samplerDesc.minFilter   = FilterMode::NEAREST;
        samplerDesc.magFilter   = FilterMode::NEAREST;
        samplerDesc.mipFilter   = MipFilterMode::NEAREST;
        samplerDesc.wrapModeU   = WrapMode::CLAMP_TO_BORDER;
        samplerDesc.wrapModeV   = WrapMode::CLAMP_TO_BORDER;
        samplerDesc.wrapModeW   = WrapMode::CLAMP_TO_BORDER;
        samplerDesc.borderColor = BorderColor::OPAQUE_WHITE_FLOAT;
        AddSampler( samplerDesc );

        samplerDesc.name      = "linear_clamped_linear";
        samplerDesc.minFilter = FilterMode::LINEAR;
        samplerDesc.magFilter = FilterMode::LINEAR;
        samplerDesc.mipFilter = MipFilterMode::LINEAR;
        samplerDesc.wrapModeU = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeV = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeW = WrapMode::CLAMP_TO_EDGE;
        AddSampler( samplerDesc );

        samplerDesc.name      = "linear_repeat_linear";
        samplerDesc.minFilter = FilterMode::LINEAR;
        samplerDesc.magFilter = FilterMode::LINEAR;
        samplerDesc.mipFilter = MipFilterMode::LINEAR;
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
