#include "core/assert.hpp"
#include "graphics/graphics_api.hpp"
#include "graphics/pg_to_vulkan_types.hpp"
#include "graphics/vulkan.hpp"
#include "resource/shader.hpp"
#include "utils/logger.hpp"
#include <set>
#include <vector>

namespace Progression
{
namespace Gfx
{

    Buffer::~Buffer()
    {
        Free();
    }

    Buffer::Buffer( Buffer&& buff )
    {
        *this = std::move( buff );
    }
    Buffer& Buffer::operator=( Buffer&& buff )
    {
        Free();
        m_length       = std::move( buff.m_length );
        m_type         = std::move( buff.m_type );
        m_handle       = std::move( buff.m_handle );
        m_memory       = std::move( buff.m_memory );
        m_device       = std::move( buff.m_device );
        buff.m_handle  = VK_NULL_HANDLE;

        return *this;
    }

    // void Buffer::SetData( void* src, size_t length )
    // {
    // }
    // void Buffer::SetData( void* src, size_t offset, size_t length )
    // {
    // }
    
    void* Buffer::Map()
    {
        void* data;
        vkMapMemory( m_device, m_memory, 0, m_length, 0, &data );
        return data;
    }

    void Buffer::UnMap()
    {
        vkUnmapMemory( m_device, m_memory );
    }

    void Buffer::Free()
    {
        if ( m_handle != VK_NULL_HANDLE )
        {
            vkDestroyBuffer( m_device, m_handle, nullptr );
            vkFreeMemory( m_device, m_memory, nullptr );
            m_handle = VK_NULL_HANDLE;
        }
    }

    size_t Buffer::GetLength() const
    {
        return m_length;
    }

    BufferType Buffer::GetType() const
    {
        return m_type;
    }

    VkBuffer Buffer::GetNativeHandle() const
    {
        return m_handle;
    }

    Buffer::operator bool() const
    {
        return m_handle != VK_NULL_HANDLE;
    }

    void Buffer::Bind( size_t offset ) const
    {
        vkBindBufferMemory( m_device, m_handle, m_memory, offset );
    }

    VertexInputDescriptor VertexInputDescriptor::Create( uint8_t numBinding, VertexBindingDescriptor* bindingDesc,
                                                         uint8_t numAttrib, VertexAttributeDescriptor* attribDesc )
    {
        VertexInputDescriptor desc;
        desc.m_vkBindingDescs.resize( numBinding );
        desc.m_vkAttribDescs.resize( numAttrib );
        for ( uint8_t i = 0; i < numBinding; ++i )
        {
            desc.m_vkBindingDescs[i].binding   = bindingDesc[i].binding;
            desc.m_vkBindingDescs[i].stride    = bindingDesc[i].stride;
            desc.m_vkBindingDescs[i].inputRate = PGToVulkanVertexInputRate( bindingDesc[i].inputRate );
        }

        for ( uint8_t i = 0; i < numAttrib; ++i )
        {
            desc.m_vkAttribDescs[i].location = attribDesc[i].location;
            desc.m_vkAttribDescs[i].binding  = attribDesc[i].binding;
            desc.m_vkAttribDescs[i].format   = PGToVulkanBufferDataType( attribDesc[i].format );
            desc.m_vkAttribDescs[i].offset   = attribDesc[i].offset;
        }

        desc.m_createInfo = {};
        desc.m_createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        desc.m_createInfo.vertexBindingDescriptionCount   = numBinding;
        desc.m_createInfo.pVertexBindingDescriptions      = numBinding ? desc.m_vkBindingDescs.data() : nullptr;
        desc.m_createInfo.vertexAttributeDescriptionCount = numAttrib;
        desc.m_createInfo.pVertexAttributeDescriptions    = numAttrib ? desc.m_vkAttribDescs.data() : nullptr;

        return desc;
    }

    const VkPipelineVertexInputStateCreateInfo& VertexInputDescriptor::GetNativeHandle()
    {
        return m_createInfo;
    }

    Sampler::~Sampler()
    {
        /*if ( m_nativeHandle != ~0u )
        {
            glDeleteSamplers( 1, &m_nativeHandle );
        }*/
    }

    Sampler::Sampler( Sampler&& s )
    {
        *this = std::move( s );
    }

    Sampler& Sampler::operator=( Sampler&& s )
    {
        m_desc           = std::move( s.m_desc );
        //m_nativeHandle   = std::move( s.m_nativeHandle );
        //s.m_nativeHandle = ~0u;

        return *this;
    }

    Sampler Sampler::Create( const SamplerDescriptor& desc )
    {
        Sampler sampler;

        PG_UNUSED( desc );
        PG_ASSERT( false );
        /*
        sampler.m_desc = desc;
        glGenSamplers( 1, &sampler.m_nativeHandle );

        auto nativeWrapS     = PGToOpenGLWrapMode( desc.wrapModeS );
        auto nativeWrapT     = PGToOpenGLWrapMode( desc.wrapModeS );
        auto nativeWrapR     = PGToOpenGLWrapMode( desc.wrapModeS );
        auto nativeMinFilter = PGToOpenGLFilterMode( desc.minFilter );
        auto nativeMagFilter = PGToOpenGLFilterMode( desc.magFilter );

        glSamplerParameteri( sampler.m_nativeHandle, GL_TEXTURE_WRAP_S, nativeWrapS );
        glSamplerParameteri( sampler.m_nativeHandle, GL_TEXTURE_WRAP_T, nativeWrapT );
        glSamplerParameteri( sampler.m_nativeHandle, GL_TEXTURE_WRAP_R, nativeWrapR );
        glSamplerParameteri( sampler.m_nativeHandle, GL_TEXTURE_MIN_FILTER, nativeMinFilter );
        glSamplerParameteri( sampler.m_nativeHandle, GL_TEXTURE_MAG_FILTER, nativeMagFilter );
        glSamplerParameterfv( sampler.m_nativeHandle, GL_TEXTURE_BORDER_COLOR,
                              glm::value_ptr( desc.borderColor ) );
        glSamplerParameterf( sampler.m_nativeHandle, GL_TEXTURE_MAX_ANISOTROPY, desc.maxAnisotropy );
        */

        return sampler;
    }

    void Sampler::Bind( uint32_t index ) const
    {
        PG_UNUSED( index );
        PG_ASSERT( false );
        //PG_ASSERT( m_nativeHandle != ~0u );
        //glBindSampler( index, m_nativeHandle );
    }

    void Sampler::Free()
    {
    }

    FilterMode Sampler::GetMinFilter() const
    {
        return m_desc.minFilter;
    }

    FilterMode Sampler::GetMagFilter() const
    {
        return m_desc.magFilter;
    }

    WrapMode Sampler::GetWrapModeS() const
    {
        return m_desc.wrapModeS;
    }

    WrapMode Sampler::GetWrapModeT() const
    {
        return m_desc.wrapModeT;
    }

    WrapMode Sampler::GetWrapModeR() const
    {
        return m_desc.wrapModeR;
    }

    float Sampler::GetMaxAnisotropy() const
    {
        return m_desc.maxAnisotropy;
    }

    glm::vec4 Sampler::GetBorderColor() const
    {
        return m_desc.borderColor;
    }

    /*Sampler::operator bool() const
    {
        return m_nativeHandle != ~0u;
    }*/

    int SizeOfPixelFromat( const PixelFormat& format )
    {
        int size[] =
        {
            0,  // INVALID

            1,  // R8_UNORM
            1,  // R8_SNORM
            1,  // R8_UINT
            1,  // R8_SINT
            1,  // R8_SRGB

            2,  // R8_G8_UNORM
            2,  // R8_G8_SNORM
            2,  // R8_G8_UINT
            2,  // R8_G8_SINT
            2,  // R8_G8_SRGB

            3,  // R8_G8_B8_UNORM
            3,  // R8_G8_B8_SNORM
            3,  // R8_G8_B8_UINT
            3,  // R8_G8_B8_SINT
            3,  // R8_G8_B8_SRGB

            3,  // B8_G8_R8_UNORM
            3,  // B8_G8_R8_SNORM
            3,  // B8_G8_R8_UINT
            3,  // B8_G8_R8_SINT
            3,  // B8_G8_R8_SRGB

            4,  // R8_G8_B8_A8_UNORM
            4,  // R8_G8_B8_A8_SNORM
            4,  // R8_G8_B8_A8_UINT
            4,  // R8_G8_B8_A8_SINT
            4,  // R8_G8_B8_A8_SRGB

            4,  // B8_G8_R8_A8_UNORM
            4,  // B8_G8_R8_A8_SNORM
            4,  // B8_G8_R8_A8_UINT
            4,  // B8_G8_R8_A8_SINT
            4,  // B8_G8_R8_A8_SRGB

            2,  // R16_UNORM
            2,  // R16_SNORM
            2,  // R16_UINT
            2,  // R16_SINT
            2,  // R16_FLOAT

            4,  // R16_G16_UNORM
            4,  // R16_G16_SNORM
            4,  // R16_G16_UINT
            4,  // R16_G16_SINT
            4,  // R16_G16_FLOAT

            6,  // R16_G16_B16_UNORM
            6,  // R16_G16_B16_SNORM
            6,  // R16_G16_B16_UINT
            6,  // R16_G16_B16_SINT
            6,  // R16_G16_B16_FLOAT

            8,  // R16_G16_B16_A16_UNORM
            8,  // R16_G16_B16_A16_SNORM
            8,  // R16_G16_B16_A16_UINT
            8,  // R16_G16_B16_A16_SINT
            8,  // R16_G16_B16_A16_FLOAT

            4,  // R32_UINT
            4,  // R32_SINT
            4,  // R32_FLOAT

            8,  // R32_G32_UINT
            8,  // R32_G32_SINT
            8,  // R32_G32_FLOAT

            12, // R32_G32_B32_UINT
            12, // R32_G32_B32_SINT
            12, // R32_G32_B32_FLOAT

            16, // R32_G32_B32_A32_UINT
            16, // R32_G32_B32_A32_SINT
            16, // R32_G32_B32_A32_FLOAT

            2,  // DEPTH_16_UNORM
            4,  // DEPTH_32_FLOAT
            3,  // DEPTH_16_UNORM_STENCIL_8_UINT
            4,  // DEPTH_24_UNORM_STENCIL_8_UINT
            5,  // DEPTH_32_FLOAT_STENCIL_8_UINT

            1,  // STENCIL_8_UINT

            // Pixel size for the BC formats is the size of the 4x4 block, not per pixel
            8,  // BC1_RGB_UNORM
            8,  // BC1_RGB_SRGB
            8,  // BC1_RGBA_UNORM
            8,  // BC1_RGBA_SRGB
            16, // BC2_UNORM
            16, // BC2_SRGB
            16, // BC3_UNORM
            16, // BC3_SRGB
            8,  // BC4_UNORM
            8,  // BC4_SNORM
            16, // BC5_UNORM
            16, // BC5_SNORM
            16, // BC6H_UFLOAT
            16, // BC6H_SFLOAT
            16, // BC7_UNORM
            16, // BC7_SRGB
        };

        PG_ASSERT( static_cast< int >( format ) < static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS ) );
        static_assert( ARRAY_COUNT( size ) == static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS ) );

        return size[static_cast< int >( format )];
    }

    bool PixelFormatIsCompressed( const PixelFormat& format )
    {
        int f = static_cast< int >( format );
        return static_cast< int >( PixelFormat::BC1_RGB_UNORM ) <= f && f < static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS );
    }

    Texture::~Texture()
    {
        Free();
    }

    Texture::Texture( Texture&& tex )
    {
        *this = std::move( tex );
    }
    Texture& Texture::operator=( Texture&& tex )
    {
        m_desc              = std::move( tex.m_desc );
        //m_nativeHandle      = std::move( tex.m_nativeHandle );
        //tex.m_nativeHandle  = ~0u;

        return *this;
    }

    Texture Texture::Create( const ImageDescriptor& desc, void* data )
    {
        PG_UNUSED( desc );
        PG_UNUSED( data );
        PG_ASSERT( false );
        Texture tex;
        return tex;
    }

    void Texture::Free()
    {
        if ( m_image != VK_NULL_HANDLE )
        {
            vkDestroyImage( g_renderState.device.GetNativeHandle(), m_image, nullptr );
            m_image = VK_NULL_HANDLE;
        }

        if ( m_imageView != VK_NULL_HANDLE )
        {
            vkDestroyImageView( g_renderState.device.GetNativeHandle(), m_imageView, nullptr );
            m_imageView = VK_NULL_HANDLE;
        }
    }

    unsigned char* Texture::GetPixelData() const
    {
        PG_ASSERT( false );
        return nullptr;
    }

    ImageType Texture::GetType() const
    {
        return m_desc.type;
    }

    PixelFormat Texture::GetPixelFormat() const
    {
        return m_desc.dstFormat;
    }

    uint8_t Texture::GetMipLevels() const
    {
        return m_desc.mipLevels;
    }

    uint8_t Texture::GetArrayLayers() const
    {
        return m_desc.arrayLayers;
    }

    uint32_t Texture::GetWidth() const
    {
        return m_desc.width;
    }

    uint32_t Texture::GetHeight() const
    {
        return m_desc.height;
    }

    uint32_t Texture::GetDepth() const
    {
        return m_desc.depth;
    }

    VkImage Texture::GetNativeHandle() const
    {
        return m_image;
    }

    Texture::operator bool() const
    {
        return m_image != VK_NULL_HANDLE || m_imageView != VK_NULL_HANDLE;
    }


    // ------------- RENDER PASS ----------------//
    RenderPass::~RenderPass()
    {
        Free();
    }

    RenderPass::RenderPass( RenderPass&& r )
    {
        *this = std::move( r );
    }

    RenderPass& RenderPass::operator=( RenderPass&& r )
    {
        desc       = std::move( r.desc );
        m_handle   = std::move( r.m_handle );
        r.m_handle = VK_NULL_HANDLE;

        return *this;
    }

    RenderPass RenderPass::Create( const RenderPassDescriptor& desc )
    {
        RenderPass pass;
        pass.desc = desc;

        std::vector< VkAttachmentDescription > colorAttachments;
        std::vector< VkAttachmentReference > colorAttachmentRefs;

        for ( size_t i = 0; i < desc.colorAttachmentDescriptors.size(); ++i )
        {
            const auto& attach = desc.colorAttachmentDescriptors[i];
            if ( attach.format == PixelFormat::INVALID )
            {
                break;
            }

            colorAttachments.push_back( {} );
            colorAttachments[i].format         = PGToVulanPixelFormat( attach.format );
            colorAttachments[i].samples        = VK_SAMPLE_COUNT_1_BIT;
            colorAttachments[i].loadOp         = PGToVulkanLoadAction( attach.loadAction );
            colorAttachments[i].storeOp        = PGToVulkanStoreAction( attach.storeAction );
            colorAttachments[i].stencilLoadOp  = PGToVulkanLoadAction( LoadAction::DONT_CARE );
            colorAttachments[i].stencilStoreOp = PGToVulkanStoreAction( StoreAction::DONT_CARE );
            colorAttachments[i].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachments[i].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            colorAttachmentRefs.push_back( {} );
            colorAttachmentRefs[i].attachment = i;
            colorAttachmentRefs[i].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = static_cast< uint32_t >( colorAttachmentRefs.size() );
        subpass.pColorAttachments    = colorAttachmentRefs.data();

        VkSubpassDependency dependency = {};
        dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass    = 0;
        dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast< uint32_t >( colorAttachments.size() );
        renderPassInfo.pAttachments    = colorAttachments.data();
        renderPassInfo.subpassCount    = 1;
        renderPassInfo.pSubpasses      = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies   = &dependency;

        if ( vkCreateRenderPass( g_renderState.device.GetNativeHandle(), &renderPassInfo, nullptr, &pass.m_handle ) != VK_SUCCESS )
        {
            pass.m_handle = VK_NULL_HANDLE;
        }

        return pass;
    }

    void RenderPass::Free()
    {
        if ( m_handle != VK_NULL_HANDLE )
        {
            vkDestroyRenderPass( g_renderState.device.GetNativeHandle(), m_handle, nullptr );
            m_handle = VK_NULL_HANDLE;
        }
    }
        
    VkRenderPass RenderPass::GetNativeHandle() const
    {
        return m_handle;
    }
    
    RenderPass::operator bool() const
    {
        return m_handle != VK_NULL_HANDLE;
    }

    Pipeline::~Pipeline()
    {
        Free();
    }

    Pipeline::Pipeline( Pipeline&& pipeline )
    {
        *this = std::move ( pipeline );
    }

    Pipeline& Pipeline::operator=( Pipeline&& pipeline )
    {
        m_desc           = std::move( pipeline.m_desc );
        m_pipeline       = std::move( pipeline.m_pipeline );
        m_pipelineLayout = std::move( pipeline.m_pipelineLayout );

        pipeline.m_pipeline       = VK_NULL_HANDLE;
        pipeline.m_pipelineLayout = VK_NULL_HANDLE;

        return *this;
    }

    Pipeline Pipeline::Create( const PipelineDescriptor& desc )
    {
        Pipeline p;
        p.m_desc = desc;

        std::vector< VkPipelineShaderStageCreateInfo > shaderStages( desc.numShaders );
        for ( int i = 0; i < desc.numShaders; ++i )
        {
            PG_ASSERT( desc.shaders[i] );
            shaderStages[i] = desc.shaders[i]->GetVkPipelineShaderStageCreateInfo();
        }

        VkViewport viewport;
        viewport.x        = desc.viewport.x;
        viewport.y        = desc.viewport.y;
        viewport.width    = desc.viewport.width;
        viewport.height   = desc.viewport.height;
        viewport.minDepth = desc.viewport.minDepth;
        viewport.maxDepth = desc.viewport.maxDepth;

        VkRect2D scissor;
        scissor.offset = { desc.scissor.x, desc.scissor.y };
        scissor.extent = { (uint32_t) desc.scissor.width, (uint32_t) desc.scissor.height };

        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports    = &viewport;
        viewportState.scissorCount  = 1;
        viewportState.pScissors     = &scissor;

        // specify topology and if primitive restart is on
        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology               = PGToVulkanPrimitiveType( desc.primitiveType );
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // rasterizer does rasterization, depth testing, face culling, and scissor test 
        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable        = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode             = PGToVulkanPolygonMode( desc.rasterizerInfo.polygonMode );
        rasterizer.lineWidth               = 1.0f; // > 1 needs the wideLines GPU feature
        rasterizer.cullMode                = PGToVulkanCullFace( desc.rasterizerInfo.cullFace );
        rasterizer.frontFace               = PGToVulkanWindingOrder( desc.rasterizerInfo.winding );
        rasterizer.depthBiasEnable         = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable   = VK_FALSE;
        multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;

        // no depth or stencil buffer currently

        // blending for single attachment
        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        // blending for all attachments / global settings
        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable     = VK_FALSE;
        colorBlending.logicOp           = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount   = 1;
        colorBlending.pAttachments      = &colorBlendAttachment;

        // no dynamic state currently

        // pipeline layout where you specify uniforms (none currently)
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount         = 0;
        pipelineLayoutInfo.pSetLayouts            = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges    = nullptr;

        if ( vkCreatePipelineLayout( g_renderState.device.GetNativeHandle(), &pipelineLayoutInfo, nullptr, &p.m_pipelineLayout ) != VK_SUCCESS )
        {
            return p;
        }

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount          = static_cast< uint32_t >( shaderStages.size() );
        pipelineInfo.pStages             = shaderStages.data();
        pipelineInfo.pVertexInputState   = &p.m_desc.vertexDescriptor.GetNativeHandle();
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState      = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState   = &multisampling;
        pipelineInfo.pDepthStencilState  = nullptr;
        pipelineInfo.pColorBlendState    = &colorBlending;
        pipelineInfo.pDynamicState       = nullptr;
        pipelineInfo.layout              = p.m_pipelineLayout;
        pipelineInfo.renderPass          = desc.renderPass->GetNativeHandle();
        pipelineInfo.renderPass          = desc.renderPass->GetNativeHandle();
        pipelineInfo.subpass             = 0;
        pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;

        if ( vkCreateGraphicsPipelines( g_renderState.device.GetNativeHandle(), VK_NULL_HANDLE, 1,
                                        &pipelineInfo, nullptr, &p.m_pipeline ) != VK_SUCCESS )
        {
            vkDestroyPipelineLayout( g_renderState.device.GetNativeHandle(), p.m_pipelineLayout, nullptr );
            p.m_pipeline = VK_NULL_HANDLE;
        }

        return p;
    }

    void Pipeline::Free()
    {
        if ( m_pipeline != VK_NULL_HANDLE )
        {
            vkDestroyPipeline( g_renderState.device.GetNativeHandle(), m_pipeline, nullptr );
            vkDestroyPipelineLayout( g_renderState.device.GetNativeHandle(), m_pipelineLayout, nullptr );
            m_pipeline       = VK_NULL_HANDLE;
            m_pipelineLayout = VK_NULL_HANDLE;
        }
    }

    VkPipeline Pipeline::GetNativeHandle() const
    {
        return m_pipeline;
    }

    Pipeline::operator bool() const
    {
        return m_pipeline != VK_NULL_HANDLE;
    }

    void Fence::Free()
    {
        vkDestroyFence( m_device, m_handle, nullptr );
    }

    void Fence::WaitFor()
    {
        PG_ASSERT( m_device != VK_NULL_HANDLE && m_handle != VK_NULL_HANDLE );
        vkWaitForFences( m_device, 1, &m_handle, VK_TRUE, UINT64_MAX );
    }

    void Fence::Reset()
    {
        PG_ASSERT( m_device != VK_NULL_HANDLE && m_handle != VK_NULL_HANDLE );
        vkResetFences( m_device, 1, &m_handle );
    }

    Device::~Device()
    {
        Free();
    }

    Device::Device( Device&& device )
    {
        *this = std::move( device );
    }

    Device& Device::operator=( Device&& device )
    {
        m_handle        = device.m_handle;
        m_graphicsQueue = device.m_graphicsQueue;
        m_presentQueue  = device.m_presentQueue;

        device.m_handle = VK_NULL_HANDLE;

        return *this;
    }

    Device Device::CreateDefault()
    {
        Device device;
        const auto& indices                     = g_renderState.physicalDeviceInfo.indices;
        VkPhysicalDeviceFeatures deviceFeatures = {};
        std::set< uint32_t> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

        float queuePriority = 1.0f;
        std::vector< VkDeviceQueueCreateInfo > queueCreateInfos;
        for ( uint32_t queueFamily : uniqueQueueFamilies )
        {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex        = queueFamily;
            queueCreateInfo.queueCount              = 1;
            queueCreateInfo.pQueuePriorities        = &queuePriority;
            queueCreateInfos.push_back( queueCreateInfo );
        }

        VkDeviceCreateInfo createInfo      = {};
        createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount    = static_cast< uint32_t >( queueCreateInfos.size() );
        createInfo.pQueueCreateInfos       = queueCreateInfos.data();
        createInfo.pEnabledFeatures        = &deviceFeatures;
        createInfo.enabledExtensionCount   = static_cast< uint32_t >( VK_DEVICE_EXTENSIONS.size() );
        createInfo.ppEnabledExtensionNames = VK_DEVICE_EXTENSIONS.data();

        // Specify device specific validation layers (ignored after v1.1.123)
    #if !USING( SHIP_BUILD )
        createInfo.enabledLayerCount   = static_cast< uint32_t >( VK_VALIDATION_LAYERS.size() );
        createInfo.ppEnabledLayerNames = VK_VALIDATION_LAYERS.data();
    #else // #if !USING( SHIP_BUILD )
        createInfo.enabledLayerCount   = 0;
    #endif // #else // #if !USING( SHIP_BUILD )

        if ( vkCreateDevice( g_renderState.physicalDeviceInfo.device, &createInfo, nullptr, &device.m_handle ) != VK_SUCCESS )
        {
            return {};
        }

        vkGetDeviceQueue( device.m_handle, indices.graphicsFamily, 0, &device.m_graphicsQueue );
        vkGetDeviceQueue( device.m_handle, indices.presentFamily,  0, &device.m_presentQueue );

        return device;
    }

    void Device::Free()
    {
        if ( m_handle != VK_NULL_HANDLE )
        {
            vkDestroyDevice( m_handle, nullptr );
            m_handle = VK_NULL_HANDLE;
        }
    }

    CommandPool Device::NewCommandPool() const
    {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = g_renderState.physicalDeviceInfo.indices.graphicsFamily;
        poolInfo.flags            = 0;

        CommandPool cmdPool;
        cmdPool.m_device = m_handle;
        if ( vkCreateCommandPool( m_handle, &poolInfo, nullptr, &cmdPool.m_handle ) != VK_SUCCESS )
        {
            cmdPool.m_handle = VK_NULL_HANDLE;
        }

        return cmdPool;
    }

    Fence Device::NewFence() const
    {
        Fence fence;
        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        fence.m_device  = m_handle;
        VkResult ret    = vkCreateFence( m_handle, &fenceInfo, nullptr, &fence.m_handle );
        PG_ASSERT( ret == VK_SUCCESS );

        return fence;
    }

    // Buffer Device::NewBuffer( void* data, size_t length, BufferType type ) const
    Buffer Device::NewBuffer( size_t length, BufferType type ) const
    {
        Buffer buffer;
        buffer.m_device = m_handle;
        buffer.m_type   = type;
        buffer.m_length = length;

        VkBufferCreateInfo info = {};
        info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.usage       = PGToVulkanBufferType( type );
        info.size        = length;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VkResult ret     = vkCreateBuffer( m_handle, &info, nullptr, &buffer.m_handle );
        PG_ASSERT( ret == VK_SUCCESS );

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements( m_handle, buffer.m_handle, &memRequirements );

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize       = memRequirements.size;
        VkMemoryPropertyFlags flags    = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        allocInfo.memoryTypeIndex      = FindMemoryType( memRequirements.memoryTypeBits, flags );
        ret = vkAllocateMemory( m_handle, &allocInfo, nullptr, &buffer.m_memory );
        PG_ASSERT( ret == VK_SUCCESS );

        vkBindBufferMemory( m_handle, buffer.m_handle, buffer.m_memory, 0 );

        return buffer;
    }

    VkDevice Device::GetNativeHandle() const
    {
        return m_handle;
    }

    VkQueue Device::GraphicsQueue() const
    {
        return m_graphicsQueue;
    }

    VkQueue Device::PresentQueue() const
    {
        return m_presentQueue;
    }

    Device::operator bool() const
    {
        return m_handle != VK_NULL_HANDLE;
    }

    void Device::SubmitRenderCommands( int numBuffers, CommandBuffer* cmdBufs ) const
    {
        PG_ASSERT( 0 <= numBuffers && numBuffers <= 5 );
        VkCommandBuffer vkCmdBufs[5];
        for ( int i = 0; i < numBuffers; ++i )
        {
            vkCmdBufs[i] = cmdBufs[i].GetNativeHandle();
        }

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[]      = { g_renderState.presentCompleteSemaphores[g_renderState.currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount     = 1;
        submitInfo.pWaitSemaphores        = waitSemaphores;
        submitInfo.pWaitDstStageMask      = waitStages;
        submitInfo.commandBufferCount     = numBuffers;
        submitInfo.pCommandBuffers        = vkCmdBufs;

        VkSemaphore signalSemaphores[]    = { g_renderState.renderCompleteSemaphores[g_renderState.currentFrame] };
        submitInfo.signalSemaphoreCount   = 1;
        submitInfo.pSignalSemaphores      = signalSemaphores;

        VkResult ret = vkQueueSubmit( m_graphicsQueue, 1, &submitInfo, g_renderState.inFlightFences[g_renderState.currentFrame] );
        PG_ASSERT( ret == VK_SUCCESS );
    }

    void Device::SubmitFrame( uint32_t imageIndex ) const
    {
        VkSemaphore signalSemaphores[] = { g_renderState.renderCompleteSemaphores[g_renderState.currentFrame] };
        VkPresentInfoKHR presentInfo   = {};
        presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = signalSemaphores;

        VkSwapchainKHR swapChains[] = { g_renderState.swapChain.swapChain };
        presentInfo.swapchainCount  = 1;
        presentInfo.pSwapchains     = swapChains;
        presentInfo.pImageIndices   = &imageIndex;

        vkQueuePresentKHR( m_presentQueue, &presentInfo );
        
        g_renderState.currentFrame = ( g_renderState.currentFrame + 1 ) % MAX_FRAMES_IN_FLIGHT;
        // vkDeviceWaitIdle( g_renderState.device.GetNativeHandle() );
    }

    CommandBuffer::CommandBuffer( CommandBuffer&& cmdbuf )
    {
        *this = std::move( cmdbuf );
    }

    CommandBuffer& CommandBuffer::operator=( CommandBuffer&& cmdbuf )
    {
        m_handle    = cmdbuf.m_handle;
        m_beginInfo = cmdbuf.m_beginInfo;

        cmdbuf.m_handle = VK_NULL_HANDLE;

        return *this;
    }
    
    CommandBuffer::operator bool() const
    {
        return m_handle != VK_NULL_HANDLE;
    }
    
    VkCommandBuffer CommandBuffer::GetNativeHandle() const
    {
        return m_handle;
    }

    bool CommandBuffer::BeginRecording()
    {
        PG_ASSERT( m_handle != VK_NULL_HANDLE );
        m_beginInfo = {};
        m_beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        m_beginInfo.flags            = 0;
        m_beginInfo.pInheritanceInfo = nullptr;
        return vkBeginCommandBuffer( m_handle, &m_beginInfo ) == VK_SUCCESS;
    }

    bool CommandBuffer::EndRecording()
    {
        return vkEndCommandBuffer( m_handle ) == VK_SUCCESS;
    }

    void CommandBuffer::BeginRenderPass( const RenderPass& renderPass, VkFramebuffer framebuffer )
    {
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass        = renderPass.GetNativeHandle();
        renderPassInfo.framebuffer       = framebuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = g_renderState.swapChain.extent;

        const auto& col = renderPass.desc.colorAttachmentDescriptors[0].clearColor;
        VkClearValue clearColor        = { col.r, col.g, col.b, col.a };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues    = &clearColor;

        vkCmdBeginRenderPass( m_handle, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );
    }

    void CommandBuffer::EndRenderPass()
    {
        vkCmdEndRenderPass( m_handle );
    }

    void CommandBuffer::BindRenderPipeline( const Pipeline& pipeline )
    {
        vkCmdBindPipeline( m_handle, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetNativeHandle() );
    }
    
    void CommandBuffer::Draw( uint32_t firstVert, uint32_t vertCount, uint32_t instanceCount, uint32_t firstInstance )
    {
        vkCmdDraw( m_handle, vertCount, instanceCount, firstVert, firstInstance );
    }


    CommandPool::~CommandPool()
    {
        Free();
    }

    CommandPool::CommandPool( CommandPool&& pool )
    {
        *this = std::move( pool );
    }

    CommandPool& CommandPool::operator=( CommandPool&& pool )
    {
        if ( m_handle != VK_NULL_HANDLE )
        {
            vkDestroyCommandPool( m_device, m_handle, nullptr );
        }
        
        m_handle = pool.m_handle;
        m_device = pool.m_device;
        pool.m_handle = VK_NULL_HANDLE;

        return *this;
    }

    void CommandPool::Free()
    {
        if ( m_handle != VK_NULL_HANDLE )
        {
            vkDestroyCommandPool( m_device, m_handle, nullptr );
            m_handle = VK_NULL_HANDLE;
        }
    }

    CommandPool::operator bool() const
    {
        return m_handle != VK_NULL_HANDLE;
    }

    CommandBuffer CommandPool::NewCommandBuffer()
    {
        PG_ASSERT( m_handle != VK_NULL_HANDLE );
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool        = m_handle;
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
    
        CommandBuffer buf;
        if ( vkAllocateCommandBuffers( m_device, &allocInfo, &buf.m_handle ) != VK_SUCCESS )
        {
            buf.m_handle = VK_NULL_HANDLE;
        }

        return buf;
    }

} // namespace Gfx
} // namespace Progression
