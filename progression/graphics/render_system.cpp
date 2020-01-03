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
#include <random>
#include <unordered_map>

using namespace Progression;
using namespace Gfx;

static std::unordered_map< std::string, Gfx::Sampler > s_samplers;

int g_debugLayer = 0;

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
    DescriptorSet ssao;
    DescriptorSet ssaoBlur;
    DescriptorSet lights;
    DescriptorSet background;
    DescriptorSet postProcessInputColorTex;
} descriptorSets;

RenderSystem::ShadowPassData shadowPassData;

RenderSystem::GBufferPassData gBufferPassData;

struct
{
    RenderPass renderPass;
    Texture texture;
    Texture noise;
    Buffer kernel;
    Framebuffer frameBuffer;
    Pipeline pipeline;
    std::vector< DescriptorSetLayout > descriptorSetLayouts;
} ssaoPassData;

struct
{
    Pipeline pipeline;
    RenderPass renderPass;
    std::vector< DescriptorSetLayout > descriptorSetLayouts;
    Texture outputTex;
    Framebuffer framebuffer;
} ssaoBlurPassData;

struct
{
    RenderPass renderPass;
    Texture outputTexture;
    Framebuffer frameBuffer;
    Pipeline pipeline;
    std::vector< DescriptorSetLayout > descriptorSetLayouts;
} lightingPassData;

struct
{
    RenderPass renderPass;
    Pipeline skyboxPipeline;
    Buffer cubeBuffer;
    std::vector< DescriptorSetLayout > skyboxDescriptorSetLayouts;
    Pipeline solidColorPipeline;
} backgroundPassData;

struct
{
    RenderPass renderPass;
    Pipeline pipeline;
    std::vector< DescriptorSetLayout > descriptorSetLayouts;
} transparencyPassData;

struct
{
	Pipeline pipeline;
    Buffer quadBuffer;
    std::vector< DescriptorSetLayout > descriptorSetLayouts;
} postProcessPassData;

#define MAX_NUM_POINT_LIGHTS 1024
#define MAX_NUM_SPOT_LIGHTS 256

static bool InitShadowPassData()
{
    RenderPassDescriptor desc;
    desc.depthAttachmentDescriptor.format      = PixelFormat::DEPTH_32_FLOAT;
    desc.depthAttachmentDescriptor.loadAction  = LoadAction::CLEAR;
    desc.depthAttachmentDescriptor.storeAction = StoreAction::STORE;
    desc.depthAttachmentDescriptor.finalLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    
    shadowPassData.renderPass = g_renderState.device.NewRenderPass( desc, "directional shadow" );
    if ( !shadowPassData.renderPass )
    {
        return false;
    }

    auto vertShader = ResourceManager::Get< Shader >( "directionalShadowVert" );
    PG_ASSERT( vertShader );

    VertexBindingDescriptor bindingDescs[] =
    {
        VertexBindingDescriptor( 0, sizeof( glm::vec3 ) ),
        VertexBindingDescriptor( 1, 2 * sizeof( glm::vec4 ) ),
    };

    VertexAttributeDescriptor attribDescs[] =
    {
        VertexAttributeDescriptor( 0, 0, BufferDataType::FLOAT3, 0 ),
        VertexAttributeDescriptor( 1, 1, BufferDataType::FLOAT4, 0 ),
        VertexAttributeDescriptor( 2, 1, BufferDataType::UINT4, sizeof( glm::vec4 ) ),
    };

    PipelineDescriptor shadowPassDataPipelineDesc;
    shadowPassDataPipelineDesc.rasterizerInfo.depthBiasEnable = true;
    shadowPassDataPipelineDesc.renderPass             = &shadowPassData.renderPass;
    shadowPassDataPipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 1, bindingDescs, 1, attribDescs );
    shadowPassDataPipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    // shadowPassDataPipelineDesc.viewport               = Viewport( DIRECTIONAL_SHADOW_MAP_RESOLUTION, -DIRECTIONAL_SHADOW_MAP_RESOLUTION );
    // shadowPassDataPipelineDesc.viewport.y             = DIRECTIONAL_SHADOW_MAP_RESOLUTION;
    // shadowPassDataPipelineDesc.scissor                = Scissor( DIRECTIONAL_SHADOW_MAP_RESOLUTION, DIRECTIONAL_SHADOW_MAP_RESOLUTION );
    shadowPassDataPipelineDesc.shaders[0]             = vertShader.get();
    shadowPassDataPipelineDesc.dynamicStates          = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    shadowPassData.rigidPipeline = g_renderState.device.NewPipeline( shadowPassDataPipelineDesc, "directional shadow pass rigid" );
    if ( !shadowPassData.rigidPipeline )
    {
        LOG_ERR( "Could not create directional shadow rigid pipeline" );
        return false;
    }

    vertShader = ResourceManager::Get< Shader >( "directionalShadowAnimatedVert" );
    PG_ASSERT( vertShader );

    shadowPassData.animatedDescriptorSetLayouts = g_renderState.device.NewDescriptorSetLayouts( vertShader->reflectInfo.descriptorSetLayouts );

    shadowPassDataPipelineDesc.descriptorSetLayouts = shadowPassData.animatedDescriptorSetLayouts;
    shadowPassDataPipelineDesc.vertexDescriptor     = VertexInputDescriptor::Create( 2, bindingDescs, 3, attribDescs );
    shadowPassDataPipelineDesc.shaders[0]           = vertShader.get();

    shadowPassData.animatedPipeline = g_renderState.device.NewPipeline( shadowPassDataPipelineDesc, "directional shadow pass animated" );
    if ( !shadowPassData.animatedPipeline )
    {
        LOG_ERR( "Could not create directional shadow animated pipeline" );
        return false;
    }

    return true;
}

static bool InitGBufferPassData()
{
    RenderPassDescriptor desc;
    desc.colorAttachmentDescriptors[0].format      = PixelFormat::R32_G32_B32_A32_FLOAT;
    desc.colorAttachmentDescriptors[0].finalLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    desc.colorAttachmentDescriptors[1].format      = PixelFormat::R8_G8_B8_A8_UNORM; // R16_G16_B16_A16_FLOAT // R8_G8_B8_A8_UNORM
    desc.colorAttachmentDescriptors[1].finalLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    desc.colorAttachmentDescriptors[2].format      = PixelFormat::R16_G16_B16_A16_UINT;
    desc.colorAttachmentDescriptors[2].finalLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;

    desc.depthAttachmentDescriptor.format      = PixelFormat::DEPTH_32_FLOAT;
    desc.depthAttachmentDescriptor.loadAction  = LoadAction::CLEAR;
    desc.depthAttachmentDescriptor.storeAction = StoreAction::STORE;
    desc.depthAttachmentDescriptor.finalLayout = ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    gBufferPassData.renderPass = g_renderState.device.NewRenderPass( desc, "gBuffer" );
    if ( !gBufferPassData.renderPass )
    {
        return false;
    }

    VertexBindingDescriptor bindingDescs[] =
    {
        VertexBindingDescriptor( 0, sizeof( glm::vec3 ) ),
        VertexBindingDescriptor( 1, sizeof( glm::vec3 ) ),
        VertexBindingDescriptor( 2, sizeof( glm::vec2 ) ),
        VertexBindingDescriptor( 3, sizeof( glm::vec3 ) ),
    };

    VertexAttributeDescriptor attribDescs[] =
    {
        VertexAttributeDescriptor( 0, 0, BufferDataType::FLOAT3, 0 ),
        VertexAttributeDescriptor( 1, 1, BufferDataType::FLOAT3, 0 ),
        VertexAttributeDescriptor( 2, 2, BufferDataType::FLOAT2, 0 ),
        VertexAttributeDescriptor( 3, 3, BufferDataType::FLOAT3, 0 ),
    };

    auto vertShader = ResourceManager::Get< Shader >( "rigidModelsVert" );
    auto fragShader = ResourceManager::Get< Shader >( "gBufferFrag" );
    PG_ASSERT( vertShader && fragShader );

    std::vector< DescriptorSetLayoutData > descriptorSetData = vertShader->reflectInfo.descriptorSetLayouts;
    descriptorSetData.insert( descriptorSetData.end(), fragShader->reflectInfo.descriptorSetLayouts.begin(), fragShader->reflectInfo.descriptorSetLayouts.end() );
    auto combined = CombineDescriptorSetLayouts( descriptorSetData );
    gBufferPassData.descriptorSetLayouts = g_renderState.device.NewDescriptorSetLayouts( combined );

    PipelineDescriptor pipelineDesc;
    pipelineDesc.renderPass             = &gBufferPassData.renderPass;
    pipelineDesc.descriptorSetLayouts   = gBufferPassData.descriptorSetLayouts;
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 4, bindingDescs, 4, attribDescs );
    pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.viewport               = FullScreenViewport();
    pipelineDesc.viewport.height        = -pipelineDesc.viewport.height;
    pipelineDesc.viewport.y             = -pipelineDesc.viewport.height;
    pipelineDesc.scissor                = FullScreenScissor();
    pipelineDesc.shaders[0]             = vertShader.get();
    pipelineDesc.shaders[1]             = fragShader.get();

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
    info.format                                = PixelFormat::R32_G32_B32_A32_FLOAT;
    gBufferPassData.gbuffer.positions          = g_renderState.device.NewTexture( info, false, "gbuffer position" );
    info.format                                = PixelFormat::R8_G8_B8_A8_UNORM; // R16_G16_B16_A16_FLOAT // R8_G8_B8_A8_UNORM
    gBufferPassData.gbuffer.normals            = g_renderState.device.NewTexture( info, false, "gbuffer normals" );
    info.format                                = PixelFormat::R16_G16_B16_A16_UINT;
    gBufferPassData.gbuffer.diffuseAndSpecular = g_renderState.device.NewTexture( info, false, "gbuffer diffuse + specular" );

    gBufferPassData.frameBuffer = g_renderState.device.NewFramebuffer( {
        &gBufferPassData.gbuffer.positions,
        &gBufferPassData.gbuffer.normals,
        &gBufferPassData.gbuffer.diffuseAndSpecular,
        &g_renderState.depthTex
        }, gBufferPassData.renderPass, "gbuffer pass" );

    if ( !gBufferPassData.frameBuffer )
    {
        return false;
    }

    return true;
}

static bool InitSSAOPass()
{
    RenderPassDescriptor desc;
    desc.colorAttachmentDescriptors[0].format      = PixelFormat::R8_UNORM;
    desc.colorAttachmentDescriptors[0].finalLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    
    ssaoPassData.renderPass = g_renderState.device.NewRenderPass( desc, "ssao" );
    if ( !ssaoPassData.renderPass )
    {
        return false;
    }

    auto vertShader = ResourceManager::Get< Shader >( "fullScreenQuadVert" );
    auto fragShader = ResourceManager::Get< Shader >( "ssao" );
    PG_ASSERT( vertShader && fragShader );

    VertexBindingDescriptor bindingDescs[] =
    {
        VertexBindingDescriptor( 0, sizeof( glm::vec3 ) ),
    };

    VertexAttributeDescriptor attribDescs[] =
    {
        VertexAttributeDescriptor( 0, 0, BufferDataType::FLOAT3, 0 ),
    };

    std::vector< DescriptorSetLayoutData > descriptorSetData = vertShader->reflectInfo.descriptorSetLayouts;
    descriptorSetData.insert( descriptorSetData.end(), fragShader->reflectInfo.descriptorSetLayouts.begin(), fragShader->reflectInfo.descriptorSetLayouts.end() );
    auto combined = CombineDescriptorSetLayouts( descriptorSetData );
    ssaoPassData.descriptorSetLayouts = g_renderState.device.NewDescriptorSetLayouts( combined );

    PipelineDescriptor pipelineDesc;
    pipelineDesc.renderPass           = &ssaoPassData.renderPass;
    pipelineDesc.descriptorSetLayouts = ssaoPassData.descriptorSetLayouts;
    pipelineDesc.vertexDescriptor     = VertexInputDescriptor::Create( 1, bindingDescs, 1, attribDescs );
    pipelineDesc.viewport             = FullScreenViewport();
    pipelineDesc.scissor              = FullScreenScissor();
    pipelineDesc.shaders[0]           = vertShader.get();
    pipelineDesc.shaders[1]           = fragShader.get();

    ssaoPassData.pipeline = g_renderState.device.NewPipeline( pipelineDesc, "SSAO pass" );
    if ( !ssaoPassData.pipeline )
    {
        LOG_ERR( "Could not create SSAO pipeline" );
        return false;
    }

    ImageDescriptor info;
    info.type    = ImageType::TYPE_2D;
    info.width   = g_renderState.swapChain.extent.width;
    info.height  = g_renderState.swapChain.extent.height;
    info.sampler = "nearest_clamped_nearest";
    info.usage   = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    info.format  = PixelFormat::R8_UNORM;
    ssaoPassData.texture = g_renderState.device.NewTexture( info, false, "ssao" );

    ssaoPassData.frameBuffer = g_renderState.device.NewFramebuffer( { &ssaoPassData.texture }, ssaoPassData.renderPass, "ssao pass" );
    if ( !ssaoPassData.frameBuffer )
    {
        return false;
    }

    std::uniform_real_distribution< float > randomFloats( 0.0f, 1.0f );
    std::default_random_engine generator;
    std::vector< glm::vec4 > kernel( PG_SSAO_KERNEL_SIZE );
    for ( int i = 0; i < PG_SSAO_KERNEL_SIZE; ++i )
    {
        glm::vec3 sample( randomFloats( generator ) * 2 - 1, randomFloats( generator ) * 2 - 1, randomFloats( generator ) );
        sample      = randomFloats( generator ) * glm::normalize( sample );
        float scale = i / (float) PG_SSAO_KERNEL_SIZE;
        float t     = scale * scale;
        scale       = 0.1f + t * 0.9f;
        kernel[i]   = glm::vec4( scale * sample, 0 );
    }
    ssaoPassData.kernel = g_renderState.device.NewBuffer( sizeof( glm::vec4 ) * PG_SSAO_KERNEL_SIZE, kernel.data(),
        BUFFER_TYPE_UNIFORM, MEMORY_TYPE_DEVICE_LOCAL, "SSAO Kernel" );

    std::vector< glm::vec4 > noise( 16 );
    for ( int i = 0; i < 16; ++i )
    {
        noise[i] = glm::vec4( randomFloats( generator ) * 2 - 1, randomFloats( generator ) * 2 - 1, 0, 0 );
    }

    info.type          = ImageType::TYPE_2D;
    info.width         = 4;
    info.height        = 4;
    info.sampler       = "nearest_repeat_nearest";
    info.usage         = VK_IMAGE_USAGE_SAMPLED_BIT;
    info.format        = PixelFormat::R32_G32_B32_A32_FLOAT;
    ssaoPassData.noise = g_renderState.device.NewTextureFromBuffer( info, noise.data(), false, "ssao noise" );

    return true;
}

static bool InitSSAOBlurPassData()
{
    RenderPassDescriptor desc;
    desc.colorAttachmentDescriptors[0].format      = PixelFormat::R8_UNORM;
    desc.colorAttachmentDescriptors[0].finalLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;

    ssaoBlurPassData.renderPass = g_renderState.device.NewRenderPass( desc, "ssao blur" );
    if ( !ssaoBlurPassData.renderPass )
    {
        return false;
    }

    auto vertShader = ResourceManager::Get< Shader >( "fullScreenQuadVert" );
    auto fragShader = ResourceManager::Get< Shader >( "ssaoBlur" );
    PG_ASSERT( vertShader && fragShader );

    VertexBindingDescriptor bindingDescs[] =
    {
        VertexBindingDescriptor( 0, sizeof( glm::vec3 ) ),
    };

    VertexAttributeDescriptor attribDescs[] =
    {
        VertexAttributeDescriptor( 0, 0, BufferDataType::FLOAT3, 0 ),
    };

    std::vector< DescriptorSetLayoutData > descriptorSetData = vertShader->reflectInfo.descriptorSetLayouts;
    descriptorSetData.insert( descriptorSetData.end(), fragShader->reflectInfo.descriptorSetLayouts.begin(), fragShader->reflectInfo.descriptorSetLayouts.end() );
    auto combined = CombineDescriptorSetLayouts( descriptorSetData );
    ssaoBlurPassData.descriptorSetLayouts = g_renderState.device.NewDescriptorSetLayouts( combined );

    PipelineDescriptor pipelineDesc;
    pipelineDesc.renderPass           = &ssaoBlurPassData.renderPass;
    pipelineDesc.descriptorSetLayouts = ssaoBlurPassData.descriptorSetLayouts;
    pipelineDesc.vertexDescriptor     = VertexInputDescriptor::Create( 1, bindingDescs, 1, attribDescs );
    pipelineDesc.viewport             = FullScreenViewport();
    pipelineDesc.scissor              = FullScreenScissor();
    pipelineDesc.shaders[0]           = vertShader.get();
    pipelineDesc.shaders[1]           = fragShader.get();

    ssaoBlurPassData.pipeline = g_renderState.device.NewPipeline( pipelineDesc, "SSAO blur pass" );
    if ( !ssaoBlurPassData.pipeline )
    {
        LOG_ERR( "Could not create SSAO blur pipeline" );
        return false;
    }

    ImageDescriptor info;
    info.type    = ImageType::TYPE_2D;
    info.width   = g_renderState.swapChain.extent.width;
    info.height  = g_renderState.swapChain.extent.height;
    info.sampler = "nearest_clamped_nearest";
    info.usage   = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    info.format  = PixelFormat::R8_UNORM;
    ssaoBlurPassData.outputTex = g_renderState.device.NewTexture( info, false, "ssao blur" );

    ssaoBlurPassData.framebuffer = g_renderState.device.NewFramebuffer( { &ssaoBlurPassData.outputTex }, ssaoBlurPassData.renderPass, "ssao blur pass" );
    if ( !ssaoBlurPassData.framebuffer )
    {
        return false;
    }

    return true;
}

static bool InitLightingPassData()
{
    RenderPassDescriptor desc;
    desc.colorAttachmentDescriptors[0].format      = PixelFormat::R8_G8_B8_A8_UNORM;
    desc.colorAttachmentDescriptors[0].finalLayout = ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
    //desc.colorAttachmentDescriptors[0].finalLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;

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

    VertexBindingDescriptor bindingDescs[] =
    {
        VertexBindingDescriptor( 0, sizeof( glm::vec3 ) ),
    };

    VertexAttributeDescriptor attribDescs[] =
    {
        VertexAttributeDescriptor( 0, 0, BufferDataType::FLOAT3, 0 ),
    };

    auto vertShader	 = ResourceManager::Get< Shader >( "fullScreenQuadVert" );
    auto fragShader  = ResourceManager::Get< Shader >( "deferredLightingPassFrag" );
    PG_ASSERT( vertShader && fragShader );

    std::vector< DescriptorSetLayoutData > descriptorSetData = vertShader->reflectInfo.descriptorSetLayouts;
    descriptorSetData.insert( descriptorSetData.end(), fragShader->reflectInfo.descriptorSetLayouts.begin(), fragShader->reflectInfo.descriptorSetLayouts.end() );
    auto combined = CombineDescriptorSetLayouts( descriptorSetData );
    lightingPassData.descriptorSetLayouts = g_renderState.device.NewDescriptorSetLayouts( combined );

    PipelineDescriptor pipelineDesc;
    pipelineDesc.renderPass                  = &lightingPassData.renderPass;
    pipelineDesc.descriptorSetLayouts        = lightingPassData.descriptorSetLayouts;
    pipelineDesc.vertexDescriptor            = VertexInputDescriptor::Create( 1, bindingDescs, 1, attribDescs );
    pipelineDesc.rasterizerInfo.winding      = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.viewport                    = FullScreenViewport();
    pipelineDesc.scissor                     = FullScreenScissor();
    pipelineDesc.shaders[0]                  = vertShader.get();
    pipelineDesc.shaders[1]                  = fragShader.get();
    pipelineDesc.depthInfo.depthWriteEnabled = false;
    pipelineDesc.depthInfo.depthTestEnabled  = false;

    lightingPassData.pipeline = g_renderState.device.NewPipeline( pipelineDesc, "lighting pass rigid model" );
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

static bool InitBackgroundPassData()
{
    RenderPassDescriptor desc;
    desc.colorAttachmentDescriptors[0].format        = PixelFormat::R8_G8_B8_A8_UNORM;
    desc.colorAttachmentDescriptors[0].loadAction    = LoadAction::LOAD;
    desc.colorAttachmentDescriptors[0].initialLayout = ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
    desc.colorAttachmentDescriptors[0].finalLayout   = ImageLayout::SHADER_READ_ONLY_OPTIMAL;

    desc.depthAttachmentDescriptor.format        = PixelFormat::DEPTH_32_FLOAT;
    desc.depthAttachmentDescriptor.loadAction    = LoadAction::LOAD;
    desc.depthAttachmentDescriptor.storeAction   = StoreAction::STORE;
    desc.depthAttachmentDescriptor.initialLayout = ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    desc.depthAttachmentDescriptor.finalLayout   = ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    backgroundPassData.renderPass = g_renderState.device.NewRenderPass( desc, "background pass" );
    if ( !backgroundPassData.renderPass )
    {
        return false;
    }

    VertexBindingDescriptor bindingDescs[] =
    {
        VertexBindingDescriptor( 0, sizeof( glm::vec3 ) ),
    };

    VertexAttributeDescriptor attribDescs[] =
    {
        VertexAttributeDescriptor( 0, 0, BufferDataType::FLOAT3, 0 ),
    };

    auto vertShader = ResourceManager::Get< Shader >( "backgroundSolidColorVert" );
    auto fragShader = ResourceManager::Get< Shader >( "backgroundSolidColorFrag" );
    PG_ASSERT( vertShader && fragShader );

    PipelineDescriptor pipelineDesc;
    pipelineDesc.renderPass             = &backgroundPassData.renderPass;
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( ARRAY_COUNT( bindingDescs ), bindingDescs, ARRAY_COUNT( attribDescs ), attribDescs );
    pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.viewport               = FullScreenViewport();
    pipelineDesc.scissor                = FullScreenScissor();
    pipelineDesc.shaders[0]             = vertShader.get();
    pipelineDesc.shaders[1]             = fragShader.get();

    pipelineDesc.depthInfo.compareFunc  = CompareFunction::LEQUAL;

    backgroundPassData.solidColorPipeline = g_renderState.device.NewPipeline( pipelineDesc, "background pass solid color" );
    if ( !backgroundPassData.solidColorPipeline )
    {
        LOG_ERR( "Could not create background pass solid color pipeline" );
        return false;
    }

    vertShader = ResourceManager::Get< Shader >( "backgroundSkyboxVert" );
    fragShader = ResourceManager::Get< Shader >( "backgroundSkyboxFrag" );
    PG_ASSERT( vertShader && fragShader );

    std::vector< DescriptorSetLayoutData > descriptorSetData = vertShader->reflectInfo.descriptorSetLayouts;
    descriptorSetData.insert( descriptorSetData.end(), fragShader->reflectInfo.descriptorSetLayouts.begin(), fragShader->reflectInfo.descriptorSetLayouts.end() );
    auto combined = CombineDescriptorSetLayouts( descriptorSetData );
    backgroundPassData.skyboxDescriptorSetLayouts = g_renderState.device.NewDescriptorSetLayouts( combined );

    pipelineDesc.descriptorSetLayouts   = backgroundPassData.skyboxDescriptorSetLayouts;
    pipelineDesc.rasterizerInfo.winding = WindingOrder::CLOCKWISE;
    pipelineDesc.viewport               = FullScreenViewport();
    pipelineDesc.viewport.height        = -pipelineDesc.viewport.height;
    pipelineDesc.viewport.y             = -pipelineDesc.viewport.height;
    pipelineDesc.scissor                = FullScreenScissor();
    pipelineDesc.shaders[0]             = vertShader.get();
    pipelineDesc.shaders[1]             = fragShader.get();

    backgroundPassData.skyboxPipeline = g_renderState.device.NewPipeline( pipelineDesc, "background pass skybox" );
    if ( !backgroundPassData.skyboxPipeline )
    {
        LOG_ERR( "Could not create background pass skybox pipeline" );
        return false;
    }

    glm::vec3 verts[] =
    {
        glm::vec3( -1, -1, -1 ), glm::vec3( -1,  1, -1 ), glm::vec3(  1,  1, -1 ), // front
        glm::vec3( -1, -1, -1 ), glm::vec3(  1,  1, -1 ), glm::vec3(  1, -1, -1 ), // front

        glm::vec3( -1, 1, 1 ), glm::vec3( -1,  -1, 1 ), glm::vec3(  1,  -1, 1 ), // back
        glm::vec3( -1, 1, 1 ), glm::vec3(  1,  -1, 1 ), glm::vec3(  1, 1, 1 ), // back

        glm::vec3( -1, -1, 1 ), glm::vec3( -1,  1, 1 ),  glm::vec3( -1,  1, -1 ), // left
        glm::vec3( -1, -1, 1 ), glm::vec3( -1,  1, -1 ), glm::vec3( -1, -1, -1 ), // left

        glm::vec3( 1, -1, -1 ), glm::vec3( 1,  1, -1 ), glm::vec3(  1,  1, 1 ), // right
        glm::vec3( 1, -1, -1 ), glm::vec3(  1,  1, 1 ), glm::vec3(  1, -1, 1 ), // right

        glm::vec3( -1, -1, 1 ), glm::vec3( -1, -1, -1 ), glm::vec3(  1,  -1, -1 ), // top
        glm::vec3( -1, -1, 1 ), glm::vec3(  1, -1, -1 ), glm::vec3(  1, -1, 1 ), // top

        glm::vec3( -1, 1, -1 ), glm::vec3( -1, 1, 1 ), glm::vec3(  1, 1, -1 ), // bottom
        glm::vec3( 1, 1, -1 ), glm::vec3(  -1, 1, 1 ), glm::vec3(  1, 1, 1 ), // bottom
    };

    backgroundPassData.cubeBuffer = g_renderState.device.NewBuffer( sizeof( verts ), verts, BUFFER_TYPE_VERTEX, MEMORY_TYPE_DEVICE_LOCAL, "RenderSystem Cube VBO" );

    return true;
}

static bool InitTransparencyPassData()
{
    RenderPassDescriptor desc;
    desc.colorAttachmentDescriptors[0].format        = PixelFormat::R8_G8_B8_A8_UNORM;
    desc.colorAttachmentDescriptors[0].loadAction    = LoadAction::LOAD;
    desc.colorAttachmentDescriptors[0].initialLayout = ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
    desc.colorAttachmentDescriptors[0].finalLayout   = ImageLayout::SHADER_READ_ONLY_OPTIMAL;

    desc.depthAttachmentDescriptor.format        = PixelFormat::DEPTH_32_FLOAT;
    desc.depthAttachmentDescriptor.loadAction    = LoadAction::LOAD;
    desc.depthAttachmentDescriptor.storeAction   = StoreAction::STORE;
    desc.depthAttachmentDescriptor.initialLayout = ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    desc.depthAttachmentDescriptor.finalLayout   = ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    transparencyPassData.renderPass = g_renderState.device.NewRenderPass( desc, "transparency pass" );
    if ( !transparencyPassData.renderPass )
    {
        return false;
    }
    VertexBindingDescriptor bindingDescs[] =
    {
        VertexBindingDescriptor( 0, sizeof( glm::vec3 ) ),
        VertexBindingDescriptor( 1, sizeof( glm::vec3 ) ),
        VertexBindingDescriptor( 2, sizeof( glm::vec2 ) ),
        VertexBindingDescriptor( 3, sizeof( glm::vec3 ) ),
    };

    VertexAttributeDescriptor attribDescs[] =
    {
        VertexAttributeDescriptor( 0, 0, BufferDataType::FLOAT3, 0 ),
        VertexAttributeDescriptor( 1, 1, BufferDataType::FLOAT3, 0 ),
        VertexAttributeDescriptor( 2, 2, BufferDataType::FLOAT2, 0 ),
        VertexAttributeDescriptor( 3, 3, BufferDataType::FLOAT3, 0 ),
    };

    auto vertShader = ResourceManager::Get< Shader >( "rigidModelsVert" );
    auto fragShader = ResourceManager::Get< Shader >( "forwardBlinnPhongFrag" );

    std::vector< DescriptorSetLayoutData > descriptorSetData = vertShader->reflectInfo.descriptorSetLayouts;
    descriptorSetData.insert( descriptorSetData.end(), fragShader->reflectInfo.descriptorSetLayouts.begin(), fragShader->reflectInfo.descriptorSetLayouts.end() );
    auto combined = CombineDescriptorSetLayouts( descriptorSetData );
    transparencyPassData.descriptorSetLayouts = g_renderState.device.NewDescriptorSetLayouts( combined );

    PipelineDescriptor pipelineDesc;
    pipelineDesc.renderPass             = &transparencyPassData.renderPass;
    pipelineDesc.descriptorSetLayouts   = transparencyPassData.descriptorSetLayouts;
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 4, bindingDescs, 4, attribDescs );
    pipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    pipelineDesc.viewport               = FullScreenViewport();
    pipelineDesc.viewport.height        = -pipelineDesc.viewport.height;
    pipelineDesc.viewport.y             = -pipelineDesc.viewport.height;
    pipelineDesc.scissor                = FullScreenScissor();
    pipelineDesc.shaders[0]             = vertShader.get();
    pipelineDesc.shaders[1]             = fragShader.get();

    transparencyPassData.pipeline = g_renderState.device.NewPipeline( pipelineDesc, "transparency rigid model" );
    if ( !transparencyPassData.pipeline )
    {
        LOG_ERR( "Could not create transparency pipeline" );
        return false;
    }

    return true;
}

bool InitPostProcessPassData()
{
	VertexBindingDescriptor bindingDescs[] =
    {
        VertexBindingDescriptor( 0, sizeof( glm::vec3 ) ),
    };

    VertexAttributeDescriptor attribDescs[] =
    {
        VertexAttributeDescriptor( 0, 0, BufferDataType::FLOAT3, 0 ),
    };

    auto vertShader = ResourceManager::Get< Shader >( "fullScreenQuadVert" );
    auto fragShader = ResourceManager::Get< Shader >( "postProcessFrag" );

    std::vector< DescriptorSetLayoutData > descriptorSetData = vertShader->reflectInfo.descriptorSetLayouts;
    descriptorSetData.insert( descriptorSetData.end(), fragShader->reflectInfo.descriptorSetLayouts.begin(), fragShader->reflectInfo.descriptorSetLayouts.end() );
    auto combined = CombineDescriptorSetLayouts( descriptorSetData );
    postProcessPassData.descriptorSetLayouts = g_renderState.device.NewDescriptorSetLayouts( combined );

	PipelineDescriptor postProcessingPipelineDesc;
    postProcessingPipelineDesc.renderPass	          = &g_renderState.renderPass;
    postProcessingPipelineDesc.descriptorSetLayouts   = postProcessPassData.descriptorSetLayouts;
    postProcessingPipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 1, bindingDescs, 1, attribDescs );
    postProcessingPipelineDesc.rasterizerInfo.winding = WindingOrder::COUNTER_CLOCKWISE;
    postProcessingPipelineDesc.viewport               = FullScreenViewport();
    postProcessingPipelineDesc.scissor                = FullScreenScissor();
    postProcessingPipelineDesc.shaders[0]	          = vertShader.get();
    postProcessingPipelineDesc.shaders[1]			  = fragShader.get();

	postProcessPassData.pipeline = g_renderState.device.NewPipeline( postProcessingPipelineDesc , "post process" );
	if ( !postProcessPassData.pipeline )
	{
		LOG_ERR( "Could not create post processing pipeline" );
		return false;
	}

    glm::vec3 verts[] =
    {
        glm::vec3( -1, -1, 0 ),
        glm::vec3( -1,  1, 0 ),
        glm::vec3(  1,  1, 0 ),
        glm::vec3( -1, -1, 0 ),
        glm::vec3(  1,  1, 0 ),
        glm::vec3(  1, -1, 0 ),
    };

    postProcessPassData.quadBuffer = g_renderState.device.NewBuffer( sizeof( verts ), verts, BUFFER_TYPE_VERTEX, MEMORY_TYPE_DEVICE_LOCAL, "RenderSystem Quad VBO" );

    return true;
}

bool InitDescriptorPoolAndPrimarySets()
{
    VkDescriptorPoolSize poolSize[3] = {};
    poolSize[0] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 }; // scene const buffer + ssao kernel
    poolSize[1] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 }; // point and spot light buffers
    poolSize[2] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 11 }; // tex array + 4 gbuffer attachment sampler2Ds + 3 ssao + 1 skybox

    s_descriptorPool = g_renderState.device.NewDescriptorPool( 3, poolSize, 8, "render system" );

    descriptorSets.scene                    = s_descriptorPool.NewDescriptorSet( lightingPassData.descriptorSetLayouts[0],    "scene data" );
    descriptorSets.arrayOfTextures          = s_descriptorPool.NewDescriptorSet( lightingPassData.descriptorSetLayouts[1],    "array of textures" );
    descriptorSets.gBufferAttachments       = s_descriptorPool.NewDescriptorSet( lightingPassData.descriptorSetLayouts[2],    "gbuffer attachments" );
    descriptorSets.ssao                     = s_descriptorPool.NewDescriptorSet( ssaoPassData.descriptorSetLayouts[0],        "ssao textures" );
    descriptorSets.ssaoBlur                 = s_descriptorPool.NewDescriptorSet( ssaoBlurPassData.descriptorSetLayouts[0],    "ssao blur tex" );
    descriptorSets.lights                   = s_descriptorPool.NewDescriptorSet( lightingPassData.descriptorSetLayouts[3],    "scene lights" );
    descriptorSets.postProcessInputColorTex = s_descriptorPool.NewDescriptorSet( postProcessPassData.descriptorSetLayouts[0], "post process input tex" );
    descriptorSets.background               = s_descriptorPool.NewDescriptorSet( backgroundPassData.skyboxDescriptorSetLayouts[0], "background skybox tex" );
    
    std::vector< VkWriteDescriptorSet > writeDescriptorSets;
	std::vector< VkDescriptorImageInfo > imageDescriptors;
	std::vector< VkDescriptorBufferInfo > bufferDescriptors;

    auto dummyImage = ResourceManager::Get< Image >( "RENDER_SYSTEM_DUMMY_TEXTURE" );
    PG_ASSERT( dummyImage );

    // GBuffer Pass (some shared with lighting pass)
    imageDescriptors = std::vector< VkDescriptorImageInfo >( PG_MAX_NUM_TEXTURES, DescriptorImageInfo( *dummyImage->GetTexture(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ) );

    bufferDescriptors =
    {
        DescriptorBufferInfo( s_gpuSceneConstantBuffers ),
        DescriptorBufferInfo( s_gpuPointLightBuffers ),
        DescriptorBufferInfo( s_gpuSpotLightBuffers ),
    };
    writeDescriptorSets =
    {
        WriteDescriptorSet( descriptorSets.scene,           VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,          0, &bufferDescriptors[0] ),
        WriteDescriptorSet( descriptorSets.arrayOfTextures, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,  0, imageDescriptors.data(), static_cast< uint32_t >( imageDescriptors.size() ) ),
        WriteDescriptorSet( descriptorSets.lights,          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,          1, &bufferDescriptors[1] ),
        WriteDescriptorSet( descriptorSets.lights,          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,          2, &bufferDescriptors[2] ),
    };
    g_renderState.device.UpdateDescriptorSets( static_cast< uint32_t >( writeDescriptorSets.size() ), writeDescriptorSets.data() );

    // SSAO Pass
    bufferDescriptors =
    {
        DescriptorBufferInfo( ssaoPassData.kernel ),
    };
    imageDescriptors =
    {
        DescriptorImageInfo( gBufferPassData.gbuffer.positions, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ),
        DescriptorImageInfo( gBufferPassData.gbuffer.normals,   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ),
        DescriptorImageInfo( ssaoPassData.noise,                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ),
        DescriptorImageInfo( ssaoPassData.texture,              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ),
    };
    writeDescriptorSets =
    {
        WriteDescriptorSet( descriptorSets.ssao,     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &imageDescriptors[0] ),
        WriteDescriptorSet( descriptorSets.ssao,     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &imageDescriptors[1] ),
        WriteDescriptorSet( descriptorSets.ssao,     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &imageDescriptors[2] ),
        WriteDescriptorSet( descriptorSets.ssao,     VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         3, &bufferDescriptors[0] ),
        WriteDescriptorSet( descriptorSets.ssaoBlur, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &imageDescriptors[3] ),
    };
    g_renderState.device.UpdateDescriptorSets( static_cast< uint32_t >( writeDescriptorSets.size() ), writeDescriptorSets.data() );

    // Lighting Pass
    imageDescriptors =
    {
        DescriptorImageInfo( gBufferPassData.gbuffer.positions,          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ),
        DescriptorImageInfo( gBufferPassData.gbuffer.normals,            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ),
        DescriptorImageInfo( gBufferPassData.gbuffer.diffuseAndSpecular, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ),
        DescriptorImageInfo( ssaoBlurPassData.outputTex,                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ),
    };
    writeDescriptorSets =
    {
        WriteDescriptorSet( descriptorSets.gBufferAttachments, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &imageDescriptors[0] ),
        WriteDescriptorSet( descriptorSets.gBufferAttachments, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &imageDescriptors[1] ),
        WriteDescriptorSet( descriptorSets.gBufferAttachments, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &imageDescriptors[2] ),
        WriteDescriptorSet( descriptorSets.gBufferAttachments, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, &imageDescriptors[3] ),
    };
    g_renderState.device.UpdateDescriptorSets( static_cast< uint32_t >( writeDescriptorSets.size() ), writeDescriptorSets.data() );

    // Post Process Pass
    VkDescriptorImageInfo imageInfo = DescriptorImageInfo( lightingPassData.outputTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
    writeDescriptorSets =
    {
        WriteDescriptorSet( descriptorSets.postProcessInputColorTex, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &imageInfo ),
    };
    g_renderState.device.UpdateDescriptorSets( static_cast< uint32_t >( writeDescriptorSets.size() ), writeDescriptorSets.data() );

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
        s_gpuSceneConstantBuffers = g_renderState.device.NewBuffer( sizeof( Gpu::SceneConstantBufferData ),
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

        if ( !InitSSAOPass() )
        {
            LOG_ERR( "Could not init SSAO pass data" );
            return false;
        }

        if ( !InitSSAOBlurPassData() )
        {
            LOG_ERR( "Could not init SSAO blur pass data" );
            return false;
        }

        if ( !InitLightingPassData() )
        {
            LOG_ERR( "Could not init lighting pass data" );
            return false;
        }

        if ( !InitBackgroundPassData() )
        {
            LOG_ERR( "Could not init background pass data" );
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

        if ( !Profile::Init() )
        {
            LOG_ERR( "Could not initialize profiler" );
            return false;
        }

        if ( !UIOverlay::Init() )
        {
            LOG_ERR( "Could not initialize the UIOverlay" );
            return false;
        }

        s_gpuSceneConstantBuffers.Map();
        s_gpuPointLightBuffers.Map();
        s_gpuSpotLightBuffers.Map();

        UIOverlay::AddDrawFunction( "Render Debugger", [&]()
        {
            ImGui::SetNextWindowPos( ImVec2( 5, 5 ), ImGuiCond_FirstUseEver );
		    ImGui::Begin( "Renderer Debug Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize );
            UIOverlay::ComboBox( "View", &g_debugLayer, { "Regular", "Ambient", "Diffuse", "Specular", "No SSAO", "SSAO Only", "Positions", "Normals" } );
		    ImGui::End();
        });

        return true;
    }

    void Shutdown()
    {
        g_renderState.device.WaitForIdle();
        
        UIOverlay::Shutdown();
        Profile::Shutdown();

        s_gpuSceneConstantBuffers.UnMap();
        s_gpuPointLightBuffers.UnMap();
        s_gpuSpotLightBuffers.UnMap();

        shadowPassData.renderPass.Free();
        shadowPassData.rigidPipeline.Free();
        shadowPassData.animatedPipeline.Free();
        FreeDescriptorSetLayouts( shadowPassData.animatedDescriptorSetLayouts );

        s_descriptorPool.Free();

        s_gpuSceneConstantBuffers.Free();
        s_gpuPointLightBuffers.Free();
        s_gpuSpotLightBuffers.Free();

        gBufferPassData.gbuffer.positions.Free();
        gBufferPassData.gbuffer.normals.Free();
        gBufferPassData.gbuffer.diffuseAndSpecular.Free();
        gBufferPassData.renderPass.Free();
        gBufferPassData.frameBuffer.Free();
        gBufferPassData.pipeline.Free();
        FreeDescriptorSetLayouts( gBufferPassData.descriptorSetLayouts );

        ssaoPassData.renderPass.Free();
        ssaoPassData.texture.Free();
        ssaoPassData.frameBuffer.Free();
        ssaoPassData.pipeline.Free();
        ssaoPassData.kernel.Free();
        ssaoPassData.noise.Free();
        FreeDescriptorSetLayouts( ssaoPassData.descriptorSetLayouts );

        ssaoBlurPassData.pipeline.Free();
        ssaoBlurPassData.renderPass.Free();
        ssaoBlurPassData.outputTex.Free();
        ssaoBlurPassData.framebuffer.Free();
        FreeDescriptorSetLayouts( ssaoBlurPassData.descriptorSetLayouts );

        lightingPassData.renderPass.Free();
        lightingPassData.outputTexture.Free();
        lightingPassData.frameBuffer.Free();
        lightingPassData.pipeline.Free();
        FreeDescriptorSetLayouts( lightingPassData.descriptorSetLayouts );

        backgroundPassData.renderPass.Free();
        backgroundPassData.cubeBuffer.Free();
        backgroundPassData.solidColorPipeline.Free();
        backgroundPassData.skyboxPipeline.Free();
        FreeDescriptorSetLayouts( backgroundPassData.skyboxDescriptorSetLayouts );
        // transparencyPassData.renderPass.Free();
        // transparencyPassData.pipeline.Free();
        // FreeDescriptorSetLayouts( transparencyPassData.descriptorSetLayouts );

        postProcessPassData.pipeline.Free();
        FreeDescriptorSetLayouts( postProcessPassData.descriptorSetLayouts );
        postProcessPassData.quadBuffer.Free();
    }
    
    void UpdateBuffersAndTextures( Scene* scene )
    {
        TextureManager::UpdateDescriptors( descriptorSets.arrayOfTextures );

        // skybox texture
        if ( scene->skybox )
        {
            VkDescriptorImageInfo imageDesc = DescriptorImageInfo( *scene->skybox->GetTexture(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
            VkWriteDescriptorSet writeSet = WriteDescriptorSet( descriptorSets.background, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &imageDesc );
            g_renderState.device.UpdateDescriptorSets( 1, &writeSet );
        }
        
        Gpu::SceneConstantBufferData scbuf;

        glm::vec3 pos( 0, 20, 10 );
        //auto entity = GetEntityByName( scene->registry, "camera" );
        //auto scriptData = scene->registry.get< ScriptComponent >( entity ).GetScriptData( "lightController" );
        //const glm::vec3& pos = scriptData->env["position"];
        auto V = glm::lookAt( pos, pos + glm::vec3( scene->directionalLight.direction ), glm::vec3( 0, 1, 0 ) );
        auto P = glm::ortho< float >( -30, 30, -30, 30, 0.0f, 100.0f );
        if ( scene->directionalLight.shadowMap )
        {
            scene->directionalLight.shadowMap->LSM = P * V;
            scbuf.LSM = scene->directionalLight.shadowMap->LSM;
        }

        scbuf.V              = scene->camera.GetV();
        scbuf.P              = scene->camera.GetP();
        scbuf.VP             = scene->camera.GetVP();
        scbuf.cameraPos      = glm::vec4( scene->camera.position, 0 );        
        scbuf.ambientColor   = glm::vec4( scene->ambientColor, 0 );
        scbuf.dirLight.colorAndIntensity = scene->directionalLight.colorAndIntensity;
        scbuf.dirLight.direction         = scene->directionalLight.direction;
        uint32_t shadowMapIndex          = PG_INVALID_TEXTURE_INDEX;
        if ( scene->directionalLight.shadowMap )
        {
            shadowMapIndex = scene->directionalLight.shadowMap->texture.GetShaderSlot();
        }
        scbuf.dirLight.shadowMapIndex.x = shadowMapIndex;
        scbuf.numPointLights = static_cast< uint32_t >( scene->pointLights.size() );
        scbuf.numSpotLights  = static_cast< uint32_t >( scene->spotLights.size() );
        memcpy( s_gpuSceneConstantBuffers.MappedPtr(), &scbuf, sizeof( Gpu::SceneConstantBufferData ) );
        //memcpy( s_gpuPointLightBuffers.MappedPtr(), scene->pointLights.data(), scene->pointLights.size() * sizeof( PointLight ) );
        //memcpy( s_gpuSpotLightBuffers.MappedPtr(), scene->spotLights.data(), scene->spotLights.size() * sizeof( SpotLight ) );

        AnimationSystem::UploadToGpu( scene );
    }

    static void RenderSingleShadow( Scene* scene, CommandBuffer& cmdBuf, const ShadowMap& shadowMap )
    {
        float width  = static_cast< float >( shadowMap.texture.GetWidth() );
        float height = static_cast< float >( shadowMap.texture.GetHeight() );
        Viewport viewport( width, -height );
        viewport.y = height;
        Scissor scissor( (int) width, (int) height );

        cmdBuf.BeginRenderPass( shadowPassData.renderPass, shadowMap.framebuffer, { shadowMap.texture.GetWidth(), shadowMap.texture.GetHeight() } );

        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "Shadow rigid models", glm::vec4( .2, .6, .4, 1 ) );
        cmdBuf.BindRenderPipeline( shadowPassData.rigidPipeline );
        cmdBuf.SetViewport( viewport );
        cmdBuf.SetScissor( scissor );
        cmdBuf.SetDepthBias( shadowMap.constantBias, 0, shadowMap.slopeBias );
        scene->registry.view< ModelRenderer, Transform >().each( [&]( ModelRenderer& renderer, Transform& transform )
        {
            const auto& model = renderer.model;
            auto MVP = shadowMap.LSM * transform.GetModelMatrix();
            cmdBuf.PushConstants( shadowPassData.rigidPipeline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( glm::mat4 ), &MVP[0][0] );
        
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetVertexOffset(), 0 );
            cmdBuf.BindIndexBuffer(  model->indexBuffer, model->GetIndexType() );
        
            for ( size_t i = 0; i < model->meshes.size(); ++i )
            {
                const auto& mesh = model->meshes[i];
                PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw \"" + model->name + "\" : \"" + mesh.name + "\"", glm::vec4( 0 ) );
                cmdBuf.DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
            }
        });
        PG_DEBUG_MARKER_END_REGION( cmdBuf );

        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "Shadow animated models", glm::vec4( .6, .2, .4, 1 ) );
        cmdBuf.BindRenderPipeline( shadowPassData.animatedPipeline );
        cmdBuf.SetViewport( viewport );
        cmdBuf.SetScissor( scissor );
        cmdBuf.SetDepthBias( shadowMap.constantBias, 0, shadowMap.slopeBias );
        cmdBuf.BindDescriptorSets( 1, &AnimationSystem::renderData.animationBonesDescriptorSet, shadowPassData.animatedPipeline, PG_BONE_TRANSFORMS_SET );
        scene->registry.view< Animator, SkinnedRenderer, Transform >().each( [&]( Animator& animator, SkinnedRenderer& renderer, Transform& transform )
        {
            const auto& model = renderer.model;
            Gpu::AnimatedShadowPerObjectData pushData{ shadowMap.LSM * transform.GetModelMatrix(), animator.GetTransformSlot() };
            cmdBuf.PushConstants( shadowPassData.animatedPipeline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( Gpu::AnimatedShadowPerObjectData ), &pushData );
        
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetVertexOffset(), 0 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetBlendWeightOffset(), 1 );
            cmdBuf.BindIndexBuffer(  model->indexBuffer, model->GetIndexType() );
        
            for ( size_t i = 0; i < model->meshes.size(); ++i )
            {
                const auto& mesh = model->meshes[i];
                PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw \"" + model->name + "\" : \"" + mesh.name + "\"", glm::vec4( 0 ) );
                cmdBuf.DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
            }
        });
        PG_DEBUG_MARKER_END_REGION( cmdBuf );

        cmdBuf.EndRenderPass();
    }

    void ShadowPass( Scene* scene, CommandBuffer& cmdBuf )
    {
        PG_PROFILE_TIMESTAMP( cmdBuf, "Shadow_Start" );
        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "Shadow Pass", glm::vec4( .2, .2, .4, 1 ) );
        
        if ( scene->directionalLight.shadowMap )
        {
            RenderSingleShadow( scene, cmdBuf, *scene->directionalLight.shadowMap );
        }

        PG_DEBUG_MARKER_END_REGION( cmdBuf );
        PG_PROFILE_TIMESTAMP( cmdBuf, "Shadow_End" );
    }

    void GBufferPass( Scene* scene, CommandBuffer& cmdBuf )
    {
        PG_PROFILE_TIMESTAMP( cmdBuf, "GBuffer_Start" );
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
            Gpu::ObjectConstantBufferData b{ M, N };
            cmdBuf.PushConstants( gBufferPassData.pipeline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( Gpu::ObjectConstantBufferData ), &b );

            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetVertexOffset(), 0 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetNormalOffset(), 1 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetUVOffset(), 2 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetTangentOffset(), 3 );
            cmdBuf.BindIndexBuffer(  model->indexBuffer, model->GetIndexType() );

            for ( size_t i = 0; i < model->meshes.size(); ++i )
            {
                const auto& mesh = modelRenderer.model->meshes[i];
                const auto& mat  = modelRenderer.materials[mesh.materialIndex];
                // if ( mat->transparent )
                // {
                //     continue;
                // }

                Gpu::MaterialConstantBufferData mcbuf{};
                mcbuf.Kd = glm::vec4( mat->Kd, 0 );
                mcbuf.Ks = glm::vec4( mat->Ks, mat->Ns );
                mcbuf.diffuseTexIndex = mat->map_Kd   ? mat->map_Kd->GetTexture()->GetShaderSlot()   : PG_INVALID_TEXTURE_INDEX;
                mcbuf.normalMapIndex  = mat->map_Norm ? mat->map_Norm->GetTexture()->GetShaderSlot() : PG_INVALID_TEXTURE_INDEX;
                cmdBuf.PushConstants( gBufferPassData.pipeline, VK_SHADER_STAGE_FRAGMENT_BIT, PG_MATERIAL_PUSH_CONSTANT_OFFSET, sizeof( Gpu::MaterialConstantBufferData ), &mcbuf );

                PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw \"" + model->name + "\" : \"" + mesh.name + "\"", glm::vec4( 0 ) );
                cmdBuf.DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
            }
        });
        PG_DEBUG_MARKER_END_REGION( cmdBuf );

        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "GBuffer animated models", glm::vec4( .8, .2, .2, 1 ) );
        auto& animPipeline = AnimationSystem::renderData.animatedPipeline;
        cmdBuf.BindRenderPipeline( animPipeline );
        cmdBuf.BindDescriptorSets( 1, &descriptorSets.scene, animPipeline, PG_SCENE_CONSTANT_BUFFER_SET );
        cmdBuf.BindDescriptorSets( 1, &descriptorSets.arrayOfTextures, animPipeline, PG_2D_TEXTURES_SET );
        cmdBuf.BindDescriptorSets( 1, &AnimationSystem::renderData.animationBonesDescriptorSet, animPipeline, PG_BONE_TRANSFORMS_SET );
        scene->registry.view< Animator, SkinnedRenderer, Transform >().each( [&]( Animator& animator, SkinnedRenderer& renderer, Transform& transform )
        {
            const auto& model = renderer.model;
            // TODO: Actually fix this for models without tangets as well
            if ( model->GetTangentOffset() == ~0u )
            {
                return;
            }
            
            auto M = transform.GetModelMatrix();
            auto N = glm::transpose( glm::inverse( M ) );
            Gpu::AnimatedObjectConstantBufferData b{ M, N, animator.GetTransformSlot() };
            cmdBuf.PushConstants( animPipeline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( Gpu::AnimatedObjectConstantBufferData ), &b );

            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetVertexOffset(), 0 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetNormalOffset(), 1 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetUVOffset(), 2 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetTangentOffset(), 3 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetBlendWeightOffset(), 4 );
            cmdBuf.BindIndexBuffer(  model->indexBuffer, model->GetIndexType() );

            for ( size_t i = 0; i < model->meshes.size(); ++i )
            {
                const auto& mesh = renderer.model->meshes[i];
                const auto& mat  = renderer.materials[mesh.materialIndex];
                // if ( mat->transparent )
                // {
                //     continue;
                // }

                Gpu::MaterialConstantBufferData mcbuf{};
                mcbuf.Kd = glm::vec4( mat->Kd, 0 );
                mcbuf.Ks = glm::vec4( mat->Ks, mat->Ns );
                mcbuf.diffuseTexIndex = mat->map_Kd   ? mat->map_Kd->GetTexture()->GetShaderSlot()   : PG_INVALID_TEXTURE_INDEX;
                mcbuf.normalMapIndex  = mat->map_Norm ? mat->map_Norm->GetTexture()->GetShaderSlot() : PG_INVALID_TEXTURE_INDEX;
                cmdBuf.PushConstants( animPipeline, VK_SHADER_STAGE_FRAGMENT_BIT, PG_MATERIAL_PUSH_CONSTANT_OFFSET, sizeof( Gpu::MaterialConstantBufferData ), &mcbuf );

                PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw \"" + model->name + "\" : \"" + mesh.name + "\"", glm::vec4( 0 ) );
                cmdBuf.DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
            }
        });
        PG_DEBUG_MARKER_END_REGION( cmdBuf );
        
        cmdBuf.EndRenderPass();
        PG_DEBUG_MARKER_END_REGION( cmdBuf );
        PG_PROFILE_TIMESTAMP( cmdBuf, "GBuffer_End" );
    }

    void SSAOPass( Scene* scene, CommandBuffer& cmdBuf )
    {
        PG_PROFILE_TIMESTAMP( cmdBuf, "SSAO_Start" );
        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "SSAO", glm::vec4( .5, .5, .5, 1 ) );
        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "SSAO Occlusion Pass", glm::vec4( .8, .5, .5, 1 ) );
        cmdBuf.BeginRenderPass( ssaoPassData.renderPass, ssaoPassData.frameBuffer, g_renderState.swapChain.extent );
        
        cmdBuf.BindRenderPipeline( ssaoPassData.pipeline );
        cmdBuf.BindDescriptorSets( 1, &descriptorSets.ssao, ssaoPassData.pipeline, 0 );
        Gpu::SSAOShaderData pushData{ scene->camera.GetV(), scene->camera.GetP() };
        cmdBuf.PushConstants( ssaoPassData.pipeline, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof( Gpu::SSAOShaderData ), &pushData );
        PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw full-screen quad", glm::vec4( 0 ) );
        cmdBuf.BindVertexBuffer( postProcessPassData.quadBuffer, 0, 0 );
        cmdBuf.Draw( 0, 6 );
        
        cmdBuf.EndRenderPass();
        PG_DEBUG_MARKER_END_REGION( cmdBuf );

        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "SSAO Blur Pass", glm::vec4( .5, .5, .8, 1 ) );
        cmdBuf.BeginRenderPass( ssaoBlurPassData.renderPass, ssaoBlurPassData.framebuffer, g_renderState.swapChain.extent );
        
        cmdBuf.BindRenderPipeline( ssaoBlurPassData.pipeline );
        cmdBuf.BindDescriptorSets( 1, &descriptorSets.ssaoBlur, ssaoBlurPassData.pipeline, 0 );
        PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw full-screen quad", glm::vec4( 0 ) );
        cmdBuf.BindVertexBuffer( postProcessPassData.quadBuffer, 0, 0 );
        cmdBuf.Draw( 0, 6 );
        
        cmdBuf.EndRenderPass();
        PG_DEBUG_MARKER_END_REGION( cmdBuf );
        PG_DEBUG_MARKER_END_REGION( cmdBuf );
        PG_PROFILE_TIMESTAMP( cmdBuf, "SSAO_End" );
    }

    void DeferredLightingPass( Scene* scene, CommandBuffer& cmdBuf )
    {
        PG_PROFILE_TIMESTAMP( cmdBuf, "Lighting_Start" );
        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "Lighting Pass", glm::vec4( .8, 0, .8, 1 ) );
        cmdBuf.BeginRenderPass( lightingPassData.renderPass, lightingPassData.frameBuffer, g_renderState.swapChain.extent );

        cmdBuf.BindRenderPipeline( lightingPassData.pipeline );
        cmdBuf.BindDescriptorSets( 1, &descriptorSets.scene, lightingPassData.pipeline, PG_SCENE_CONSTANT_BUFFER_SET );
        cmdBuf.BindDescriptorSets( 1, &descriptorSets.arrayOfTextures, lightingPassData.pipeline, PG_2D_TEXTURES_SET );
        cmdBuf.BindDescriptorSets( 1, &descriptorSets.gBufferAttachments, lightingPassData.pipeline, 2 );
        cmdBuf.BindDescriptorSets( 1, &descriptorSets.lights, lightingPassData.pipeline, 3 );
        PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw full-screen quad", glm::vec4( 0 ) );

        cmdBuf.PushConstants( lightingPassData.pipeline, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof( int ), &g_debugLayer );
        cmdBuf.BindVertexBuffer( postProcessPassData.quadBuffer, 0, 0 );
        cmdBuf.Draw( 0, 6 );

        cmdBuf.EndRenderPass();
        PG_DEBUG_MARKER_END_REGION( cmdBuf );
        PG_PROFILE_TIMESTAMP( cmdBuf, "Lighting_End" );
    }

    void BackgroundPass( Scene* scene, CommandBuffer& cmdBuf )
    {
        PG_PROFILE_TIMESTAMP( cmdBuf, "Background_Start" );
        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "Background Pass", glm::vec4( .2, .8, .8, 1 ) );
        cmdBuf.BeginRenderPass( backgroundPassData.renderPass, lightingPassData.frameBuffer, g_renderState.swapChain.extent );

        if ( scene->skybox )
        {
            cmdBuf.BindRenderPipeline( backgroundPassData.skyboxPipeline );
            PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw Skybox", glm::vec4( 0 ) );
            glm::mat4 VP = scene->camera.GetP() * glm::mat4( glm::mat3( scene->camera.GetV() ) );
            cmdBuf.PushConstants( backgroundPassData.skyboxPipeline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( glm::mat4 ), glm::value_ptr( VP ) );
            cmdBuf.BindDescriptorSets( 1, &descriptorSets.background, backgroundPassData.skyboxPipeline );
            cmdBuf.BindVertexBuffer( backgroundPassData.cubeBuffer, 0, 0 );
            cmdBuf.Draw( 0, 36 );
        }
        else
        {
            cmdBuf.BindRenderPipeline( backgroundPassData.solidColorPipeline );
            PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw Background Color Quad", glm::vec4( 0 ) );
            cmdBuf.PushConstants( backgroundPassData.solidColorPipeline, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof( glm::vec4 ), glm::value_ptr( scene->backgroundColor ) );
            cmdBuf.BindVertexBuffer( postProcessPassData.quadBuffer, 0, 0 );
            cmdBuf.Draw( 0, 6 );
        }

        cmdBuf.EndRenderPass();
        PG_DEBUG_MARKER_END_REGION( cmdBuf );
        PG_PROFILE_TIMESTAMP( cmdBuf, "Background_End" );
    }

    void TransparencyPass( Scene* scene, CommandBuffer& cmdBuf )
    {
        PG_PROFILE_TIMESTAMP( cmdBuf, "Transparency_Start" );
        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "Transparency Pass", glm::vec4( .8, .8, .2, 1 ) );
        cmdBuf.BeginRenderPass( transparencyPassData.renderPass, lightingPassData.frameBuffer, g_renderState.swapChain.extent );
        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "Transparency -- Rigid Models", glm::vec4( .2, .8, .2, 1 ) );
        cmdBuf.BindRenderPipeline( transparencyPassData.pipeline );
        cmdBuf.BindDescriptorSets( 1, &descriptorSets.scene, transparencyPassData.pipeline, PG_SCENE_CONSTANT_BUFFER_SET );
        cmdBuf.BindDescriptorSets( 1, &descriptorSets.arrayOfTextures, transparencyPassData.pipeline, PG_2D_TEXTURES_SET );
        cmdBuf.BindDescriptorSets( 1, &descriptorSets.lights, transparencyPassData.pipeline, 3 );

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
            Gpu::ObjectConstantBufferData b{ M, N };
            cmdBuf.PushConstants( transparencyPassData.pipeline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof( Gpu::ObjectConstantBufferData ), &b );

            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetVertexOffset(), 0 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetNormalOffset(), 1 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetUVOffset(), 2 );
            cmdBuf.BindVertexBuffer( model->vertexBuffer, model->GetTangentOffset(), 3 );
            cmdBuf.BindIndexBuffer(  model->indexBuffer, model->GetIndexType() );

            for ( size_t i = 0; i < model->meshes.size(); ++i )
            {
                const auto& mesh = modelRenderer.model->meshes[i];
                const auto& mat  = modelRenderer.materials[mesh.materialIndex];
                if ( !mat->transparent )
                {
                    continue;
                }

                Gpu::MaterialConstantBufferData mcbuf{};
                mcbuf.Kd = glm::vec4( mat->Kd, 0 );
                mcbuf.Ks = glm::vec4( mat->Ks, mat->Ns );
                mcbuf.diffuseTexIndex = mat->map_Kd   ? mat->map_Kd->GetTexture()->GetShaderSlot()   : PG_INVALID_TEXTURE_INDEX;
                mcbuf.normalMapIndex  = mat->map_Norm ? mat->map_Norm->GetTexture()->GetShaderSlot() : PG_INVALID_TEXTURE_INDEX;
                cmdBuf.PushConstants( transparencyPassData.pipeline, VK_SHADER_STAGE_FRAGMENT_BIT, PG_MATERIAL_PUSH_CONSTANT_OFFSET, sizeof( Gpu::MaterialConstantBufferData ), &mcbuf );

                PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw \"" + model->name + "\" : \"" + mesh.name + "\"", glm::vec4( 0 ) );
                cmdBuf.DrawIndexed( mesh.startIndex, mesh.numIndices, mesh.startVertex );
            }
        });
        PG_DEBUG_MARKER_END_REGION( cmdBuf );

        // transparent animated models?
        
        cmdBuf.EndRenderPass();
        PG_DEBUG_MARKER_END_REGION( cmdBuf );
        PG_PROFILE_TIMESTAMP( cmdBuf, "Transparency_End" );
    }

    void PostProcessPass( Scene* scene, CommandBuffer& cmdBuf, uint32_t swapChainImageIndex )
    {
        PG_PROFILE_TIMESTAMP( cmdBuf, "PostProcess_Start" );
        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "Post Process Pass", glm::vec4( .2, .2, 1, 1 ) );
        cmdBuf.BeginRenderPass( g_renderState.renderPass, g_renderState.swapChainFramebuffers[swapChainImageIndex], g_renderState.swapChain.extent );
        
        cmdBuf.BindRenderPipeline( postProcessPassData.pipeline );
        cmdBuf.BindDescriptorSets( 1, &descriptorSets.postProcessInputColorTex, postProcessPassData.pipeline, 0 );
        
        PG_DEBUG_MARKER_INSERT( cmdBuf, "Draw full-screen quad", glm::vec4( 0 ) );
        cmdBuf.BindVertexBuffer( postProcessPassData.quadBuffer, 0, 0 );
        cmdBuf.Draw( 0, 6 );
        
        PG_DEBUG_MARKER_END_REGION( cmdBuf );
        PG_PROFILE_TIMESTAMP( cmdBuf, "PostProcess_End" );
    }

    void UIPass( Scene* scene, CommandBuffer& cmdBuf, uint32_t swapChainImageIndex )
    {
        PG_PROFILE_TIMESTAMP( cmdBuf, "UI_Start" );
        PG_DEBUG_MARKER_BEGIN_REGION( cmdBuf, "UI Pass", glm::vec4( .4, .7, 9, 1 ) );
        
        UIOverlay::Draw( cmdBuf );
        
        cmdBuf.EndRenderPass();
        PG_DEBUG_MARKER_END_REGION( cmdBuf );
        PG_PROFILE_TIMESTAMP( cmdBuf, "UI_End" );
    }

    void Render( Scene* scene )
    {
        PG_ASSERT( scene != nullptr );
        PG_ASSERT( scene->pointLights.size() < MAX_NUM_POINT_LIGHTS && scene->spotLights.size() < MAX_NUM_SPOT_LIGHTS );

        auto swapChainImageIndex = g_renderState.swapChain.AcquireNextImage( g_renderState.presentCompleteSemaphore );

        UpdateBuffersAndTextures( scene );

        auto& cmdBuf = g_renderState.graphicsCommandBuffer;
        cmdBuf.BeginRecording();
        PG_PROFILE_RESET( cmdBuf );

        PG_PROFILE_TIMESTAMP( cmdBuf, "Frame_Start" );

        ShadowPass( scene, cmdBuf );
        GBufferPass( scene, cmdBuf );
        SSAOPass( scene, cmdBuf );
        DeferredLightingPass( scene, cmdBuf );
        BackgroundPass( scene, cmdBuf );
        PostProcessPass( scene, cmdBuf, swapChainImageIndex );
        UIPass( scene, cmdBuf, swapChainImageIndex );

        PG_PROFILE_TIMESTAMP( cmdBuf, "Frame_End" );

        cmdBuf.EndRecording();
        g_renderState.device.SubmitRenderCommands( 1, &cmdBuf );
        g_renderState.device.SubmitFrame( swapChainImageIndex );

        PG_PROFILE_GET_RESULTS();
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

        samplerDesc.name      = "nearest_repeat_nearest";
        samplerDesc.minFilter = FilterMode::NEAREST;
        samplerDesc.magFilter = FilterMode::NEAREST;
        samplerDesc.mipFilter = MipFilterMode::NEAREST;
        samplerDesc.wrapModeU = WrapMode::REPEAT;
        samplerDesc.wrapModeV = WrapMode::REPEAT;
        samplerDesc.wrapModeW = WrapMode::REPEAT;
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

        samplerDesc.name        = "linear_clamped_linear";
        samplerDesc.minFilter   = FilterMode::LINEAR;
        samplerDesc.magFilter   = FilterMode::LINEAR;
        samplerDesc.mipFilter   = MipFilterMode::LINEAR;
        samplerDesc.wrapModeU   = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeV   = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeW   = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.borderColor = BorderColor::OPAQUE_WHITE_FLOAT;
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
