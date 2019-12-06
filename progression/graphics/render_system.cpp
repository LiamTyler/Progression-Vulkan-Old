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
static DescriptorPool s_descriptorPool;
static Buffer s_gpuSceneConstantBuffers;
static Buffer s_gpuPointLightBuffers;
static Buffer s_gpuSpotLightBuffers;

struct
{
    DescriptorSet scene;
    DescriptorSet arrayOfTextures;
    DescriptorSet gBufferAttachments;
    DescriptorSet lights;
    DescriptorSet postProcessInputColorTex;
} descriptorSets;

struct
{
    Gfx::RenderPass renderPass;
    Gfx::Texture    depthAttachment;
    Framebuffer   frameBuffer;
    Gfx::Pipeline   pipeline;
    glm::mat4       LSM;
} shadowPassData;

struct GBuffer
{
    Gfx::Texture positions;
    Gfx::Texture normals;
    Gfx::Texture diffuse;
    Gfx::Texture specular;
};

struct
{
    Gfx::RenderPass renderPass;
    GBuffer gbuffer;
    Framebuffer frameBuffer;
    Gfx::Pipeline pipeline;
    std::vector< Gfx::DescriptorSetLayout > descriptorSetLayouts;
} gBufferPassData;

struct
{
    Gfx::RenderPass renderPass;
    Gfx::Texture outputTexture;
    Framebuffer frameBuffer;
    Gfx::Pipeline pipeline;
    std::vector< Gfx::DescriptorSetLayout > descriptorSetLayouts;
} lightingPassData;

struct
{
	Gfx::Pipeline pipeline;
    Gfx::Buffer quadBuffer;
    std::vector< Gfx::DescriptorSetLayout > descriptorSetLayouts;
} postProcessPassData;

#define MAX_NUM_POINT_LIGHTS 1024
#define MAX_NUM_SPOT_LIGHTS 256
#define DIRECTIONAL_SHADOW_MAP_RESOLUTION 4096

static bool InitShadowPassData()
{
    RenderPassDescriptor desc;
    desc.depthAttachmentDescriptor.format      = PixelFormat::DEPTH_32_FLOAT;
    desc.depthAttachmentDescriptor.loadAction  = LoadAction::CLEAR;
    desc.depthAttachmentDescriptor.storeAction = StoreAction::STORE;
    desc.depthAttachmentDescriptor.finalLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    
    shadowPassData.renderPass = g_renderState.device.NewRenderPass( desc, "shadowPassData" );
    if ( !shadowPassData.renderPass )
    {
        return false;
    }

    auto directionalShadowVert = ResourceManager::Get< Shader >( "directionalShadowVert" );

    VertexBindingDescriptor shadowPassDataBindingDesc[1];
    shadowPassDataBindingDesc[0].binding   = 0;
    shadowPassDataBindingDesc[0].stride    = sizeof(glm::vec3);
    shadowPassDataBindingDesc[0].inputRate = VertexInputRate::PER_VERTEX;

    std::array< VertexAttributeDescriptor, 1 > shadowPassDataAttribDescs;
    shadowPassDataAttribDescs[0].binding  = 0;
    shadowPassDataAttribDescs[0].location = 0;
    shadowPassDataAttribDescs[0].format   = BufferDataType::FLOAT3;
    shadowPassDataAttribDescs[0].offset   = 0;

    PipelineDescriptor shadowPassDataPipelineDesc;
    shadowPassDataPipelineDesc.rasterizerInfo.depthBiasEnable = true;
    shadowPassDataPipelineDesc.renderPass       = &shadowPassData.renderPass;
    shadowPassDataPipelineDesc.vertexDescriptor = VertexInputDescriptor::Create( 1, shadowPassDataBindingDesc, 1, shadowPassDataAttribDescs.data() );
    shadowPassDataPipelineDesc.viewport         = Viewport( DIRECTIONAL_SHADOW_MAP_RESOLUTION, -DIRECTIONAL_SHADOW_MAP_RESOLUTION );
    shadowPassDataPipelineDesc.viewport.y       = DIRECTIONAL_SHADOW_MAP_RESOLUTION;
    Scissor scissor                             = {};
    scissor.width                               = DIRECTIONAL_SHADOW_MAP_RESOLUTION;
    scissor.height                              = DIRECTIONAL_SHADOW_MAP_RESOLUTION;
    shadowPassDataPipelineDesc.scissor          = scissor;
    shadowPassDataPipelineDesc.shaders[0]       = directionalShadowVert.get();
    shadowPassDataPipelineDesc.numShaders       = 1;
    shadowPassDataPipelineDesc.numColorAttachments = 0;

    shadowPassData.pipeline = g_renderState.device.NewPipeline( shadowPassDataPipelineDesc, "directional shadow pass" );
    if ( !shadowPassData.pipeline )
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
    shadowPassData.depthAttachment = g_renderState.device.NewTexture( info, true, "directional shadowmap" );

    shadowPassData.frameBuffer = g_renderState.device.NewFramebuffer( { &shadowPassData.depthAttachment }, shadowPassData.renderPass, "directional shadow pass" );
    if ( !shadowPassData.frameBuffer )
    {
        return false;
    }

    return true;
}

static bool InitGBufferPassData()
{
    RenderPassDescriptor desc;
    desc.colorAttachmentDescriptors[0].format      = PixelFormat::R32_G32_B32_A32_FLOAT;
    desc.colorAttachmentDescriptors[0].finalLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    desc.colorAttachmentDescriptors[1].format      = PixelFormat::R32_G32_B32_A32_FLOAT;
    desc.colorAttachmentDescriptors[1].finalLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    desc.colorAttachmentDescriptors[2].format      = PixelFormat::R8_G8_B8_A8_UNORM;
    desc.colorAttachmentDescriptors[2].finalLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    desc.colorAttachmentDescriptors[3].format      = PixelFormat::R16_G16_B16_A16_FLOAT; //TODO!!! Reduce number of textures by packing both diffuse and specular into rgb and the specular exponent in a
    desc.colorAttachmentDescriptors[3].finalLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;

    desc.depthAttachmentDescriptor.format      = PixelFormat::DEPTH_32_FLOAT;
    desc.depthAttachmentDescriptor.loadAction  = LoadAction::CLEAR;
    desc.depthAttachmentDescriptor.storeAction = StoreAction::STORE;
    desc.depthAttachmentDescriptor.finalLayout = ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    gBufferPassData.renderPass = g_renderState.device.NewRenderPass( desc, "gBuffer" );
    if ( !gBufferPassData.renderPass )
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

    auto vertShader = ResourceManager::Get< Shader >( "rigidModelsVert" );
    auto fragShader = ResourceManager::Get< Shader >( "gBufferFrag" );

    std::vector< DescriptorSetLayoutData > descriptorSetData = vertShader->reflectInfo.descriptorSetLayouts;
    descriptorSetData.insert( descriptorSetData.end(), fragShader->reflectInfo.descriptorSetLayouts.begin(), fragShader->reflectInfo.descriptorSetLayouts.end() );
    auto combined = CombineDescriptorSetLayouts( descriptorSetData );
    gBufferPassData.descriptorSetLayouts = g_renderState.device.NewDescriptorSetLayouts( combined );

    PipelineDescriptor pipelineDesc;
    pipelineDesc.renderPass             = &gBufferPassData.renderPass;
    pipelineDesc.descriptorSetLayouts   = gBufferPassData.descriptorSetLayouts;
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 4, bindingDesc, 4, attribDescs );
    pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.viewport               = FullScreenViewport();
    pipelineDesc.viewport.height        = -pipelineDesc.viewport.height;
    pipelineDesc.viewport.y             = -pipelineDesc.viewport.height;
    pipelineDesc.scissor                = FullScreenScissor();
    pipelineDesc.shaders[0]             = vertShader.get();
    pipelineDesc.shaders[1]             = fragShader.get();
    pipelineDesc.numShaders             = 2;
    pipelineDesc.numColorAttachments    = 4;

    gBufferPassData.pipeline = g_renderState.device.NewPipeline( pipelineDesc, "gbuffer rigid model" );
    if ( !gBufferPassData.pipeline )
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
    info.format                       = PixelFormat::R32_G32_B32_A32_FLOAT;
    gBufferPassData.gbuffer.positions = g_renderState.device.NewTexture( info, false, "gbuffer position" );
    info.format                       = PixelFormat::R32_G32_B32_A32_FLOAT;
    gBufferPassData.gbuffer.normals   = g_renderState.device.NewTexture( info, false, "gbuffer normals" );
    info.format                       = PixelFormat::R8_G8_B8_A8_UNORM;
    gBufferPassData.gbuffer.diffuse   = g_renderState.device.NewTexture( info, false, "gbuffer diffuse" );
    info.format                       = PixelFormat::R16_G16_B16_A16_FLOAT;
    gBufferPassData.gbuffer.specular  = g_renderState.device.NewTexture( info, false, "gbuffer specular" );

    gBufferPassData.frameBuffer = g_renderState.device.NewFramebuffer( {
        &gBufferPassData.gbuffer.positions,
        &gBufferPassData.gbuffer.normals,
        &gBufferPassData.gbuffer.diffuse,
        &gBufferPassData.gbuffer.specular,
        &g_renderState.depthTex
        }, gBufferPassData.renderPass, "gbuffer pass" );

    if ( !gBufferPassData.frameBuffer )
    {
        return false;
    }

    return true;
}

static bool InitLightingPassData()
{
    RenderPassDescriptor desc;
    desc.colorAttachmentDescriptors[0].format      = PixelFormat::R8_G8_B8_A8_UNORM;
    desc.colorAttachmentDescriptors[0].finalLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;

    desc.depthAttachmentDescriptor.format        = PixelFormat::DEPTH_32_FLOAT;
    desc.depthAttachmentDescriptor.loadAction    = LoadAction::LOAD;
    desc.depthAttachmentDescriptor.storeAction   = StoreAction::STORE;
    desc.depthAttachmentDescriptor.initialLayout = ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    desc.depthAttachmentDescriptor.finalLayout   = ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    lightingPassData.renderPass = g_renderState.device.NewRenderPass( desc, "lighting pass" );
    if ( !lightingPassData.renderPass )
    {
        return false;
    }

    VertexBindingDescriptor bindingDesc[1];
    bindingDesc[0].binding   = 0;
    bindingDesc[0].stride    = sizeof( glm::vec3 );
    bindingDesc[0].inputRate = VertexInputRate::PER_VERTEX;

    VertexAttributeDescriptor attribDescs[1];
    attribDescs[0].binding  = 0;
    attribDescs[0].location = 0;
    attribDescs[0].format   = BufferDataType::FLOAT3;
    attribDescs[0].offset   = 0;

    auto vertShader	 = ResourceManager::Get< Shader >( "fullScreenQuadVert" );
    auto fragShader  = ResourceManager::Get< Shader >( "deferredLightingPassFrag" );

    std::vector< DescriptorSetLayoutData > descriptorSetData = vertShader->reflectInfo.descriptorSetLayouts;
    descriptorSetData.insert( descriptorSetData.end(), fragShader->reflectInfo.descriptorSetLayouts.begin(), fragShader->reflectInfo.descriptorSetLayouts.end() );
    auto combined = CombineDescriptorSetLayouts( descriptorSetData );
    lightingPassData.descriptorSetLayouts = g_renderState.device.NewDescriptorSetLayouts( combined );

    PipelineDescriptor pipelineDesc;
    pipelineDesc.renderPass                  = &lightingPassData.renderPass;
    pipelineDesc.descriptorSetLayouts        = lightingPassData.descriptorSetLayouts;
    pipelineDesc.vertexDescriptor            = VertexInputDescriptor::Create( 1, bindingDesc, 1, attribDescs );
    pipelineDesc.rasterizerInfo.winding      = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.viewport                    = FullScreenViewport();
    pipelineDesc.scissor                     = FullScreenScissor();
    pipelineDesc.shaders[0]                  = vertShader.get();
    pipelineDesc.shaders[1]                  = fragShader.get();
    pipelineDesc.numShaders                  = 2;
    pipelineDesc.numColorAttachments         = 1;
    pipelineDesc.depthInfo.depthWriteEnabled = false;
    pipelineDesc.depthInfo.depthTestEnabled  = false;

    lightingPassData.pipeline = g_renderState.device.NewPipeline( pipelineDesc, "gbuffer rigid model" );
    if ( !lightingPassData.pipeline )
    {
        LOG_ERR( "Could not create lighting pass pipeline" );
        return false;
    }

    ImageDescriptor info;
    info.type    = ImageType::TYPE_2D;
    info.width   = g_renderState.swapChain.extent.width;
    info.height  = g_renderState.swapChain.extent.height;
    info.sampler = "nearest_clamped_nearest";
    info.usage   = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    info.format  = PixelFormat::R8_G8_B8_A8_UNORM;
    lightingPassData.outputTexture = g_renderState.device.NewTexture( info, false, "lit scene" );

    lightingPassData.frameBuffer = g_renderState.device.NewFramebuffer( { &lightingPassData.outputTexture, &g_renderState.depthTex }, lightingPassData.renderPass, "lighting pass" );
    if ( !lightingPassData.frameBuffer )
    {
        return false;
    }

    return true;
}

bool InitPostProcessPassData()
{
	VertexBindingDescriptor postProcescsingBindingDesc[1];
    postProcescsingBindingDesc[0].binding   = 0;
    postProcescsingBindingDesc[0].stride    = sizeof( glm::vec3 );
    postProcescsingBindingDesc[0].inputRate = VertexInputRate::PER_VERTEX;

	std::array< VertexAttributeDescriptor, 1 > postProcescsingAttribDescs;
    postProcescsingAttribDescs[0].binding   = 0;
    postProcescsingAttribDescs[0].location  = 0;
    postProcescsingAttribDescs[0].format    = BufferDataType::FLOAT3;
    postProcescsingAttribDescs[0].offset    = 0;

    auto vertShader = ResourceManager::Get< Shader >( "fullScreenQuadVert" );
    auto fragShader = ResourceManager::Get< Shader >( "postProcessFrag" );

    std::vector< DescriptorSetLayoutData > descriptorSetData = vertShader->reflectInfo.descriptorSetLayouts;
    descriptorSetData.insert( descriptorSetData.end(), fragShader->reflectInfo.descriptorSetLayouts.begin(), fragShader->reflectInfo.descriptorSetLayouts.end() );
    auto combined = CombineDescriptorSetLayouts( descriptorSetData );
    postProcessPassData.descriptorSetLayouts = g_renderState.device.NewDescriptorSetLayouts( combined );

	PipelineDescriptor postProcessingPipelineDesc;
    postProcessingPipelineDesc.renderPass	          = &g_renderState.renderPass;
    postProcessingPipelineDesc.descriptorSetLayouts   = postProcessPassData.descriptorSetLayouts;
    postProcessingPipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 1, postProcescsingBindingDesc, 1, postProcescsingAttribDescs.data() );
    postProcessingPipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    postProcessingPipelineDesc.viewport               = FullScreenViewport();
    postProcessingPipelineDesc.scissor                = FullScreenScissor();
    postProcessingPipelineDesc.shaders[0]	          = vertShader.get();
    postProcessingPipelineDesc.shaders[1]			  = fragShader.get();
    postProcessingPipelineDesc.numShaders             = 2;
    postProcessingPipelineDesc.numColorAttachments    = 1;

	postProcessPassData.pipeline = g_renderState.device.NewPipeline( postProcessingPipelineDesc , "post process" );
	if ( !postProcessPassData.pipeline )
	{
		LOG_ERR( "Could not create post processing pipeline" );
		return false;
	}

    glm::vec3 quadVerts[] =
    {
        glm::vec3( -1, -1, 0 ),
        glm::vec3( -1,  1, 0 ),
        glm::vec3(  1,  1, 0 ),
        glm::vec3( -1, -1, 0 ),
        glm::vec3(  1,  1, 0 ),
        glm::vec3(  1, -1, 0 ),
    };

    postProcessPassData.quadBuffer = g_renderState.device.NewBuffer( sizeof( quadVerts ), quadVerts, BUFFER_TYPE_VERTEX, MEMORY_TYPE_DEVICE_LOCAL, "RenderSystem Quad VBO" );

    return true;
}

bool InitDescriptorPoolAndPrimarySets()
{
    VkDescriptorPoolSize poolSize[3] = {};
    poolSize[0] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }; // scene const buffer
    poolSize[1] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 }; // point and spot light buffers
    poolSize[2] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5 }; // tex array + 4 gbuffer attachment sampler2Ds

    s_descriptorPool = g_renderState.device.NewDescriptorPool( 3, poolSize, 5, "render system" );

    descriptorSets.scene                    = s_descriptorPool.NewDescriptorSet( lightingPassData.descriptorSetLayouts[0], "scene data" );
    descriptorSets.arrayOfTextures          = s_descriptorPool.NewDescriptorSet( lightingPassData.descriptorSetLayouts[1], "array of textures" );
    descriptorSets.gBufferAttachments       = s_descriptorPool.NewDescriptorSet( lightingPassData.descriptorSetLayouts[2], "gbuffer attachments" );
    descriptorSets.lights                   = s_descriptorPool.NewDescriptorSet( lightingPassData.descriptorSetLayouts[3], "scene lights" );
    descriptorSets.postProcessInputColorTex = s_descriptorPool.NewDescriptorSet( postProcessPassData.descriptorSetLayouts[0], "post process input tex" );
    
    auto dummyImage = ResourceManager::Get< Image >( "RENDER_SYSTEM_DUMMY_TEXTURE" );
    PG_ASSERT( dummyImage );
    VkDescriptorImageInfo imageInfo;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.sampler     = dummyImage->GetTexture()->GetSampler()->GetHandle();
    imageInfo.imageView   = dummyImage->GetTexture()->GetView();
    std::vector< VkDescriptorImageInfo > imageInfos( PG_MAX_NUM_TEXTURES, imageInfo );

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = s_gpuSceneConstantBuffers.GetHandle();
    bufferInfo.offset = 0;
    bufferInfo.range  = VK_WHOLE_SIZE;

    VkWriteDescriptorSet descriptorWrite[4] = {};
    descriptorWrite[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite[0].dstSet           = descriptorSets.scene.GetHandle();
    descriptorWrite[0].dstBinding       = 0;
    descriptorWrite[0].dstArrayElement  = 0;
    descriptorWrite[0].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite[0].descriptorCount  = 1;
    descriptorWrite[0].pBufferInfo      = &bufferInfo;

    descriptorWrite[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite[1].dstSet           = descriptorSets.arrayOfTextures.GetHandle();
    descriptorWrite[1].dstBinding       = 0;
    descriptorWrite[1].dstArrayElement  = 0;
    descriptorWrite[1].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite[1].descriptorCount  = static_cast< uint32_t >( imageInfos.size() );
    descriptorWrite[1].pImageInfo       = imageInfos.data();

    VkDescriptorBufferInfo bufferInfo2 = {};
    bufferInfo2.buffer = s_gpuPointLightBuffers.GetHandle();
    bufferInfo2.offset = 0;
    bufferInfo2.range  = VK_WHOLE_SIZE;
    descriptorWrite[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite[2].dstSet           = descriptorSets.lights.GetHandle();
    descriptorWrite[2].dstBinding       = 1;
    descriptorWrite[2].dstArrayElement  = 0;
    descriptorWrite[2].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrite[2].descriptorCount  = 1;
    descriptorWrite[2].pBufferInfo      = &bufferInfo2;

    VkDescriptorBufferInfo bufferInfo3 = {};
    bufferInfo3.buffer = s_gpuSpotLightBuffers.GetHandle();
    bufferInfo3.offset = 0;
    bufferInfo3.range  = VK_WHOLE_SIZE;
    descriptorWrite[3].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite[3].dstSet           = descriptorSets.lights.GetHandle();
    descriptorWrite[3].dstBinding       = 2;
    descriptorWrite[3].dstArrayElement  = 0;
    descriptorWrite[3].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrite[3].descriptorCount  = 1;
    descriptorWrite[3].pBufferInfo      = &bufferInfo3;

    g_renderState.device.UpdateDescriptorSets( 4, descriptorWrite );

    // lighting pass shader
    VkDescriptorImageInfo gbufferImageInfos[4];
    gbufferImageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    gbufferImageInfos[0].sampler     = gBufferPassData.gbuffer.positions.GetSampler()->GetHandle();
    gbufferImageInfos[0].imageView   = gBufferPassData.gbuffer.positions.GetView();
    gbufferImageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    gbufferImageInfos[1].sampler     = gBufferPassData.gbuffer.normals.GetSampler()->GetHandle();
    gbufferImageInfos[1].imageView   = gBufferPassData.gbuffer.normals.GetView();
    gbufferImageInfos[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    gbufferImageInfos[2].sampler     = gBufferPassData.gbuffer.diffuse.GetSampler()->GetHandle();
    gbufferImageInfos[2].imageView   = gBufferPassData.gbuffer.diffuse.GetView();
    gbufferImageInfos[3].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    gbufferImageInfos[3].sampler     = gBufferPassData.gbuffer.specular.GetSampler()->GetHandle();
    gbufferImageInfos[3].imageView   = gBufferPassData.gbuffer.specular.GetView();

    descriptorWrite[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite[0].dstSet           = descriptorSets.gBufferAttachments.GetHandle();
    descriptorWrite[0].dstBinding       = 0;
    descriptorWrite[0].dstArrayElement  = 0;
    descriptorWrite[0].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite[0].descriptorCount  = 1;
    descriptorWrite[0].pImageInfo       = &gbufferImageInfos[0];

    descriptorWrite[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite[1].dstSet           = descriptorSets.gBufferAttachments.GetHandle();
    descriptorWrite[1].dstBinding       = 1;
    descriptorWrite[1].dstArrayElement  = 0;
    descriptorWrite[1].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite[1].descriptorCount  = 1;
    descriptorWrite[1].pImageInfo       = &gbufferImageInfos[1];

    descriptorWrite[2].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite[2].dstSet           = descriptorSets.gBufferAttachments.GetHandle();
    descriptorWrite[2].dstBinding       = 2;
    descriptorWrite[2].dstArrayElement  = 0;
    descriptorWrite[2].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite[2].descriptorCount  = 1;
    descriptorWrite[2].pImageInfo       = &gbufferImageInfos[2];

    descriptorWrite[3].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite[3].dstSet           = descriptorSets.gBufferAttachments.GetHandle();
    descriptorWrite[3].dstBinding       = 3;
    descriptorWrite[3].dstArrayElement  = 0;
    descriptorWrite[3].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite[3].descriptorCount  = 1;
    descriptorWrite[3].pImageInfo       = &gbufferImageInfos[3];

    g_renderState.device.UpdateDescriptorSets( 4, descriptorWrite );

    // post process shader
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.sampler     = lightingPassData.outputTexture.GetSampler()->GetHandle();
    imageInfo.imageView   = lightingPassData.outputTexture.GetView();

    descriptorWrite[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite[0].dstSet           = descriptorSets.postProcessInputColorTex.GetHandle();
    descriptorWrite[0].dstBinding       = 0;
    descriptorWrite[0].dstArrayElement  = 0;
    descriptorWrite[0].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite[0].descriptorCount  = 1;
    descriptorWrite[0].pImageInfo       = &imageInfo;

    g_renderState.device.UpdateDescriptorSets( 1, descriptorWrite );

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
        s_gpuSceneConstantBuffers = g_renderState.device.NewBuffer( sizeof( SceneConstantBufferData ),
                BUFFER_TYPE_UNIFORM, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT, "Scene Constants" );
        s_gpuPointLightBuffers = g_renderState.device.NewBuffer( sizeof( PointLight ) * MAX_NUM_POINT_LIGHTS,
                BUFFER_TYPE_STORAGE, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT, "Point Lights " );
        s_gpuSpotLightBuffers = g_renderState.device.NewBuffer( sizeof( SpotLight ) * MAX_NUM_SPOT_LIGHTS,
                BUFFER_TYPE_STORAGE, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT, "Spot Lights " );

        if ( !InitGBufferPassData() )
        {
            LOG_ERR( "Could not init gbuffer pass data" );
            return false;
        }

        if ( !InitLightingPassData() )
        {
            LOG_ERR( "Could not init lighting pass data" );
            return false;
        }

        if ( !InitPostProcessPassData() )
        {
            LOG_ERR( "Could not init post process data" );
            return false;
        }

        if ( !InitShadowPassData() )
        {
            LOG_ERR( "Could not init shadow pass data" );
            return false;
        }

        if ( !InitDescriptorPoolAndPrimarySets() )
        {
            LOG_ERR( "Could not init descriptor pool and sets" );
            return false;
        }

        s_gpuSceneConstantBuffers.Map();
        s_gpuPointLightBuffers.Map();
        s_gpuSpotLightBuffers.Map();

        return true;
    }

    void Shutdown()
    {
        g_renderState.device.WaitForIdle();

        s_gpuSceneConstantBuffers.UnMap();
        s_gpuPointLightBuffers.UnMap();
        s_gpuSpotLightBuffers.UnMap();

        shadowPassData.depthAttachment.Free();
        shadowPassData.renderPass.Free();
        shadowPassData.pipeline.Free();
        shadowPassData.frameBuffer.Free();

        s_descriptorPool.Free();

        s_gpuSceneConstantBuffers.Free();
        s_gpuPointLightBuffers.Free();
        s_gpuSpotLightBuffers.Free();

        // Free GBuffer Data
        gBufferPassData.gbuffer.positions.Free();
        gBufferPassData.gbuffer.normals.Free();
        gBufferPassData.gbuffer.diffuse.Free();
        gBufferPassData.gbuffer.specular.Free();
        gBufferPassData.renderPass.Free();
        gBufferPassData.frameBuffer.Free();
        gBufferPassData.pipeline.Free();
        for ( auto& layout : gBufferPassData.descriptorSetLayouts )
        {
            layout.Free();
        }

        lightingPassData.renderPass.Free();
        lightingPassData.outputTexture.Free();
        lightingPassData.frameBuffer.Free();
        lightingPassData.pipeline.Free();
        for ( auto& layout : lightingPassData.descriptorSetLayouts )
        {
            layout.Free();
        }

        postProcessPassData.pipeline.Free();
        for ( auto& layout : postProcessPassData.descriptorSetLayouts )
        {
            layout.Free();
        }
        postProcessPassData.quadBuffer.Free();
    }

    void ShadowPass( Scene* scene, CommandBuffer& cmdBuf )
    {
        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "Shadow Pass", glm::vec4( .2, .2, .4, 1 ) );
        cmdBuf.BeginRenderPass( shadowPassData.renderPass, shadowPassData.frameBuffer, { DIRECTIONAL_SHADOW_MAP_RESOLUTION, DIRECTIONAL_SHADOW_MAP_RESOLUTION } );

        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "Shadow rigid models", glm::vec4( .2, .6, .4, 1 ) );
        cmdBuf.BindRenderPipeline( shadowPassData.pipeline );
        cmdBuf.SetDepthBias( 2, 0, 9.5 ); // Values that 'worked' (removed many artifacts) epirically for sponza.json
        scene->registry.view< ModelRenderer, Transform >().each( [&]( ModelRenderer& modelRenderer, Transform& transform )
        {
            const auto& model = modelRenderer.model;
            auto MVP = shadowPassData.LSM * transform.GetModelMatrix();
            vkCmdPushConstants( cmdBuf.GetHandle(), shadowPassData.pipeline.GetLayoutHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( glm::mat4 ), &MVP[0][0] );

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

        // PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "Shadow animated models", glm::vec4( .6, .2, .4, 1 ) );
        // auto& animPipeline = AnimationSystem::renderData.animatedPipeline;
        // scene->registry.view< Animator, SkinnedRenderer, Transform >().each( [&]( Animator& animator, SkinnedRenderer& renderer, Transform& transform )
        // {
        //     const auto& model = renderer.model;
        //     auto MVP          = shadowPassData.LSM * transform.GetModelMatrix();
        //     vkCmdPushConstants( cmdBuf.GetHandle(), animPipeline.GetLayoutHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( glm::mat4 ), &MVP[0][0] );
        // 
        //     cmdBuf.BindVertexBuffer( model->vertexBuffer, 0, 0 );
        //     cmdBuf.BindIndexBuffer(  model->indexBuffer, model->GetIndexType() );
        // 
        //     for ( size_t i = 0; i < renderer.model->meshes.size(); ++i )
        //     {
        //         const auto& mesh = model->meshes[i];
        //         PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw \"" + model->name + "\" : \"" + mesh.name + "\"", glm::vec4( 0 ) );
        //         cmdBuf.DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
        //     }
        // });
        // PG_DEBUG_MARKER_END_REGION( cmdBuf );

        cmdBuf.EndRenderPass();
        PG_DEBUG_MARKER_END_REGION( cmdBuf );
    }

    void Render( Scene* scene )
    {
        PG_ASSERT( scene != nullptr );
        PG_ASSERT( scene->pointLights.size() < MAX_NUM_POINT_LIGHTS && scene->spotLights.size() < MAX_NUM_SPOT_LIGHTS );

        auto imageIndex = g_renderState.swapChain.AcquireNextImage( g_renderState.presentCompleteSemaphore );

        TextureManager::UpdateDescriptors( descriptorSets.arrayOfTextures );

        // TODO: Remove hardcoded directional shadow map
        glm::vec3 pos( 0, 15, 0 );
        auto V = glm::lookAt( pos, pos + glm::vec3( scene->directionalLight.direction ), glm::vec3( 0, 1, 0 ) );
        float W = 25;
        auto P = glm::ortho( -W, W, -W, W, 0.0f, 50.0f );
        shadowPassData.LSM = P * V;

        // sceneConstantBuffer
        SceneConstantBufferData scbuf;
        scbuf.V              = scene->camera.GetV();
        scbuf.P              = scene->camera.GetP();
        scbuf.VP             = scene->camera.GetVP();
        scbuf.DLSM           = shadowPassData.LSM;
        scbuf.cameraPos      = glm::vec4( scene->camera.position, 0 );        
        scbuf.ambientColor   = glm::vec4( scene->ambientColor, 0 );
        scbuf.dirLight       = scene->directionalLight;
        scbuf.numPointLights = static_cast< uint32_t >( scene->pointLights.size() );
        scbuf.numSpotLights  = static_cast< uint32_t >( scene->spotLights.size() );
        scbuf.shadowTextureIndex = shadowPassData.depthAttachment.GetShaderSlot();
        memcpy( s_gpuSceneConstantBuffers.MappedPtr(), &scbuf, sizeof( SceneConstantBufferData ) );
        memcpy( s_gpuPointLightBuffers.MappedPtr(), scene->pointLights.data(), scene->pointLights.size() * sizeof( PointLight ) );
        memcpy( s_gpuSpotLightBuffers.MappedPtr(), scene->spotLights.data(), scene->spotLights.size() * sizeof( SpotLight ) );

        AnimationSystem::UploadToGpu( scene, imageIndex );

        auto& cmdBuf = g_renderState.graphicsCommandBuffer;
        cmdBuf.BeginRecording();

        ShadowPass( scene, cmdBuf );

        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "GBuffer Pass", glm::vec4( .8, .8, .2, 1 ) );
        cmdBuf.BeginRenderPass( gBufferPassData.renderPass, gBufferPassData.frameBuffer, g_renderState.swapChain.extent );
        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "GBuffer -- Rigid Models", glm::vec4( .2, .8, .2, 1 ) );
        cmdBuf.BindRenderPipeline( gBufferPassData.pipeline );
        cmdBuf.BindDescriptorSets( 1, &descriptorSets.scene, gBufferPassData.pipeline, PG_SCENE_CONSTANT_BUFFER_SET );
        cmdBuf.BindDescriptorSets( 1, &descriptorSets.arrayOfTextures, gBufferPassData.pipeline, PG_2D_TEXTURES_SET );

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
            vkCmdPushConstants( cmdBuf.GetHandle(), gBufferPassData.pipeline.GetLayoutHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( ObjectConstantBufferData ), &b );

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
                vkCmdPushConstants( cmdBuf.GetHandle(), gBufferPassData.pipeline.GetLayoutHandle(), VK_SHADER_STAGE_FRAGMENT_BIT, PG_MATERIAL_PUSH_CONSTANT_OFFSET, sizeof( MaterialConstantBufferData ), &mcbuf );

                PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw \"" + model->name + "\" : \"" + mesh.name + "\"", glm::vec4( 0 ) );
                cmdBuf.DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
            }
        });
        PG_DEBUG_MARKER_END_REGION( cmdBuf );
        
        cmdBuf.EndRenderPass(); // end gbuffer pass
        PG_DEBUG_MARKER_END_REGION( cmdBuf );


        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "Lighting Pass", glm::vec4( .8, 0, .8, 1 ) );
        cmdBuf.BeginRenderPass( lightingPassData.renderPass, lightingPassData.frameBuffer, g_renderState.swapChain.extent );

        cmdBuf.BindRenderPipeline( lightingPassData.pipeline );
        cmdBuf.BindDescriptorSets( 1, &descriptorSets.scene, lightingPassData.pipeline, PG_SCENE_CONSTANT_BUFFER_SET );
        cmdBuf.BindDescriptorSets( 1, &descriptorSets.arrayOfTextures, lightingPassData.pipeline, PG_2D_TEXTURES_SET );
        cmdBuf.BindDescriptorSets( 1, &descriptorSets.gBufferAttachments, lightingPassData.pipeline, 2 );
        cmdBuf.BindDescriptorSets( 1, &descriptorSets.lights, lightingPassData.pipeline, 3 );
        PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw full-screen quad", glm::vec4( 0 ) );
        cmdBuf.BindVertexBuffer( postProcessPassData.quadBuffer, 0, 0 );
        cmdBuf.Draw( 0, 6 );

        cmdBuf.EndRenderPass(); // end lighting pass
        PG_DEBUG_MARKER_END_REGION( cmdBuf );


        // Post Processing Render Pass
        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "Post Process Pass", glm::vec4( .2, .2, 1, 1 ) );
        cmdBuf.BeginRenderPass( g_renderState.renderPass, g_renderState.swapChainFramebuffers[imageIndex], g_renderState.swapChain.extent );
        
        cmdBuf.BindRenderPipeline( postProcessPassData.pipeline );
        cmdBuf.BindDescriptorSets( 1, &descriptorSets.postProcessInputColorTex, postProcessPassData.pipeline, 0 );
        
        PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw full-screen quad", glm::vec4( 0 ) );
        cmdBuf.BindVertexBuffer( postProcessPassData.quadBuffer, 0, 0 );
        cmdBuf.Draw( 0, 6 );
        
        cmdBuf.EndRenderPass(); // end post process pass
        PG_DEBUG_MARKER_END_REGION( cmdBuf );

        cmdBuf.EndRecording();
        g_renderState.device.SubmitRenderCommands( 1, &cmdBuf );
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
