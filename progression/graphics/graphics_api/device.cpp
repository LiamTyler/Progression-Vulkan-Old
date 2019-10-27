#include "graphics/graphics_api/device.hpp"
#include "core/assert.hpp"
#include "core/platform_defines.hpp"
#include "graphics/pg_to_vulkan_types.hpp"
#include "graphics/vulkan.hpp"
#include "utils/logger.hpp"
#include <set>

namespace Progression
{
namespace Gfx
{

    Device Device::CreateDefault()
    {
        Device device;
        const auto& indices                     = g_renderState.physicalDeviceInfo.indices;
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
        createInfo.pEnabledFeatures        = &g_renderState.physicalDeviceInfo.deviceFeatures;
        createInfo.enabledExtensionCount   = static_cast< uint32_t >( VK_DEVICE_EXTENSIONS.size() );
        createInfo.ppEnabledExtensionNames = VK_DEVICE_EXTENSIONS.data();

        // Specify device specific validation layers (ignored after v1.1.123?)
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

    void Device::Submit( const CommandBuffer& cmdBuf ) const
    {
        VkSubmitInfo submitInfo = {};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        VkCommandBuffer vkCmdBuf      = cmdBuf.GetHandle();
        submitInfo.pCommandBuffers    = &vkCmdBuf;

        vkQueueSubmit( m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
    }

    void Device::WaitForIdle() const
    {
        vkQueueWaitIdle( m_graphicsQueue );
    }

    CommandPool Device::NewCommandPool( CommandPoolCreateFlags flags ) const
    {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = g_renderState.physicalDeviceInfo.indices.graphicsFamily;
        poolInfo.flags            = PGToVulkanCommandPoolCreateFlags( flags );

        CommandPool cmdPool;
        cmdPool.m_device = m_handle;
        if ( vkCreateCommandPool( m_handle, &poolInfo, nullptr, &cmdPool.m_handle ) != VK_SUCCESS )
        {
            cmdPool.m_handle = VK_NULL_HANDLE;
        }

        return cmdPool;
    }

    DescriptorPool Device::NewDescriptorPool( int numPoolSizes, VkDescriptorPoolSize* poolSizes, uint32_t maxSets ) const
    {
        DescriptorPool pool;
        pool.m_device = m_handle;

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = numPoolSizes;
        poolInfo.pPoolSizes    = poolSizes;
        poolInfo.maxSets       = maxSets;

        VkResult ret = vkCreateDescriptorPool( m_handle, &poolInfo, nullptr, &pool.m_handle );
        PG_ASSERT( ret == VK_SUCCESS );

        return pool;
    }

    std::vector< DescriptorSetLayout > Device::NewDescriptorSetLayouts( const std::vector< DescriptorSetLayoutData >& layoutData ) const
    {
        uint32_t maxSetNumber = 0;
        for ( const auto& layout : layoutData )
        {
            maxSetNumber = std::max( maxSetNumber, layout.setNumber );
        }

        std::vector< DescriptorSetLayout > layouts( maxSetNumber + 1 );
        for ( const auto& layout : layoutData )
        {
            VkResult ret = vkCreateDescriptorSetLayout( m_handle, &layout.createInfo, nullptr, &layouts[layout.setNumber].m_handle );
            PG_ASSERT( ret == VK_SUCCESS );
        }

        VkDescriptorSetLayoutCreateInfo emptyInfo = {};
        emptyInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        emptyInfo.bindingCount = 0;
        emptyInfo.pBindings    = nullptr;

        for ( uint32_t i = 0; i < maxSetNumber + 1; ++i )
        {
            layouts[i].m_device = m_handle;
            if ( layouts[i].m_handle == VK_NULL_HANDLE )
            {
                VkResult ret = vkCreateDescriptorSetLayout( m_handle, &emptyInfo, nullptr, &layouts[i].m_handle );
                PG_ASSERT( ret == VK_SUCCESS );
            }
        }

        return layouts; 
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

    Buffer Device::NewBuffer( size_t length, BufferType type, MemoryType memoryType ) const
    {
        Buffer buffer;
        buffer.m_device       = m_handle;
        buffer.m_type         = type;
        buffer.m_memoryType   = memoryType;
        buffer.m_length       = length;

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
        VkMemoryPropertyFlags flags    = PGToVulkanMemoryType( memoryType );
        allocInfo.memoryTypeIndex      = FindMemoryType( memRequirements.memoryTypeBits, flags );
        ret = vkAllocateMemory( m_handle, &allocInfo, nullptr, &buffer.m_memory );
        PG_ASSERT( ret == VK_SUCCESS );

        vkBindBufferMemory( m_handle, buffer.m_handle, buffer.m_memory, 0 );

        return buffer;
    }

    Buffer Device::NewBuffer( size_t length, void* data, BufferType type, MemoryType memoryType ) const
    {
        Buffer dstBuffer;

        if ( memoryType & MEMORY_TYPE_DEVICE_LOCAL )
        {
            Buffer stagingBuffer = NewBuffer( length, BUFFER_TYPE_TRANSFER_SRC, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT );
            void* stagingBufferData = stagingBuffer.Map();
            memcpy( stagingBufferData, data, length );
            stagingBuffer.UnMap();

            dstBuffer = NewBuffer( length, type | BUFFER_TYPE_TRANSFER_DST, memoryType );
            Copy( dstBuffer, stagingBuffer );
            stagingBuffer.Free();
        }
        else if ( ( memoryType & MEMORY_TYPE_HOST_VISIBLE ) && ( memoryType & MEMORY_TYPE_HOST_COHERENT ) )
        {
            dstBuffer = NewBuffer( length, type, memoryType );
            void* dstBufferData = dstBuffer.Map();
            memcpy( dstBufferData, data, length );
            dstBuffer.UnMap();
        }
        else
        {
            PG_ASSERT( false, "Unknown MemoryType passed into NewBuffer. Not copying data into buffer" );
        }

        return dstBuffer;
    }

    Texture Device::NewTexture( const ImageDescriptor& desc ) const
    {
        bool isDepth = PixelFormatIsDepthFormat( desc.format );
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = PGToVulkanImageType( desc.type );
        imageInfo.extent.width  = desc.width;
        imageInfo.extent.height = desc.height;
        imageInfo.extent.depth  = desc.depth;
        imageInfo.mipLevels     = desc.mipLevels;
        imageInfo.arrayLayers   = desc.arrayLayers;
        imageInfo.format        = PGToVulkanPixelFormat( desc.format );
        imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        if ( isDepth )
        {
            imageInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags         = 0;

        Texture tex;
        tex.m_device = m_handle;
        tex.m_desc   = desc;

        VkResult res = vkCreateImage( m_handle, &imageInfo, nullptr, &tex.m_image );
        PG_ASSERT( res == VK_SUCCESS);

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements( m_handle, tex.m_image, &memRequirements );

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType( memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
        res                       = vkAllocateMemory( m_handle, &allocInfo, nullptr, &tex.m_memory );
        PG_ASSERT( res == VK_SUCCESS);
        vkBindImageMemory( m_handle, tex.m_image, tex.m_memory, 0 );

        VkFormatFeatureFlags features = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
        if ( isDepth )
        {
            features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        VkFormat vkFormat = PGToVulkanPixelFormat( desc.format );
        PG_ASSERT( FormatSupported( vkFormat, features ) );
        tex.m_imageView = CreateImageView( tex.m_image, vkFormat, isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT );

        return tex;
    }

    Sampler Device::NewSampler( const SamplerDescriptor& desc ) const
    {
        Sampler sampler;
        sampler.m_desc   = desc;
        sampler.m_device = m_handle;

        VkSamplerCreateInfo info = {};
        info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.magFilter               = PGToVulkanFilterMode( desc.magFilter );
        info.minFilter               = PGToVulkanFilterMode( desc.minFilter );
        info.addressModeU            = PGToVulkanWrapMode( desc.wrapModeU );
        info.addressModeV            = PGToVulkanWrapMode( desc.wrapModeV );
        info.addressModeW            = PGToVulkanWrapMode( desc.wrapModeW );
        info.anisotropyEnable        = desc.maxAnisotropy > 1.0f ? VK_TRUE : VK_FALSE;
        info.maxAnisotropy           = desc.maxAnisotropy;
        info.borderColor             = PGToVulkanBorderColor( desc.borderColor );
        info.unnormalizedCoordinates = VK_FALSE;
        info.compareEnable           = VK_FALSE;
        info.compareOp               = VK_COMPARE_OP_ALWAYS;
        info.mipmapMode              = PGToVulkanMipFilter( desc.mipFilter );
        info.mipLodBias              = 0.0f;
        info.minLod                  = 0.0f;
        info.maxLod                  = 0.0f;

        VkResult res = vkCreateSampler( m_handle, &info, nullptr, &sampler.m_handle );
        PG_ASSERT( res == VK_SUCCESS );

        return sampler;
    }

    Pipeline Device::NewPipeline( const PipelineDescriptor& desc ) const
    {
        Pipeline p;
        p.m_desc   = desc;
        p.m_device = m_handle;

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
        std::vector< VkDescriptorSetLayout > layouts( desc.descriptorSetLayouts.size() );
        for ( size_t i = 0; i < layouts.size(); ++i )
        {
            layouts[i] = desc.descriptorSetLayouts[i].GetHandle();
        }
        pipelineLayoutInfo.setLayoutCount         = static_cast< uint32_t >( layouts.size() );
        pipelineLayoutInfo.pSetLayouts            = layouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges    = nullptr;

        if ( vkCreatePipelineLayout( m_handle, &pipelineLayoutInfo, nullptr, &p.m_pipelineLayout ) != VK_SUCCESS )
        {
            return p;
        }

        VkPipelineDepthStencilStateCreateInfo depthStencil = {};
        depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable       = desc.depthInfo.depthTestEnabled;
        depthStencil.depthWriteEnable      = desc.depthInfo.depthWriteEnabled;
        depthStencil.depthCompareOp        = PGToVulkanCompareFunction( desc.depthInfo.compareFunc );
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable     = VK_FALSE;

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount          = static_cast< uint32_t >( shaderStages.size() );
        pipelineInfo.pStages             = shaderStages.data();
        pipelineInfo.pVertexInputState   = &p.m_desc.vertexDescriptor.GetHandle();
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState      = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState   = &multisampling;
        pipelineInfo.pDepthStencilState  = &depthStencil;
        pipelineInfo.pColorBlendState    = &colorBlending;
        pipelineInfo.pDynamicState       = nullptr;
        pipelineInfo.layout              = p.m_pipelineLayout;
        pipelineInfo.renderPass          = desc.renderPass->GetHandle();
        pipelineInfo.renderPass          = desc.renderPass->GetHandle();
        pipelineInfo.subpass             = 0;
        pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;

        if ( vkCreateGraphicsPipelines( m_handle, VK_NULL_HANDLE, 1,
                                        &pipelineInfo, nullptr, &p.m_pipeline ) != VK_SUCCESS )
        {
            vkDestroyPipelineLayout( m_handle, p.m_pipelineLayout, nullptr );
            p.m_pipeline = VK_NULL_HANDLE;
        }

        return p;
    }

    RenderPass Device::NewRenderPass( const RenderPassDescriptor& desc ) const
    {
        RenderPass pass;
        pass.desc     = desc;
        pass.m_device = m_handle;

        std::vector< VkAttachmentDescription > attachments;
        std::vector< VkAttachmentReference > attachmentRefs;

        for ( size_t i = 0; i < desc.colorAttachmentDescriptors.size(); ++i )
        {
            const auto& attach = desc.colorAttachmentDescriptors[i];
            if ( attach.format == PixelFormat::INVALID )
            {
                break;
            }

            attachments.push_back( {} );
            attachments[i].format         = PGToVulkanPixelFormat( attach.format );
            attachments[i].samples        = VK_SAMPLE_COUNT_1_BIT;
            attachments[i].loadOp         = PGToVulkanLoadAction( attach.loadAction );
            attachments[i].storeOp        = PGToVulkanStoreAction( attach.storeAction );
            attachments[i].stencilLoadOp  = PGToVulkanLoadAction( LoadAction::DONT_CARE );
            attachments[i].stencilStoreOp = PGToVulkanStoreAction( StoreAction::DONT_CARE );
            attachments[i].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[i].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            attachmentRefs.push_back( {} );
            attachmentRefs[i].attachment = static_cast< uint32_t>( i );
            attachmentRefs[i].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format         = PGToVulkanPixelFormat( desc.depthAttachmentDescriptor.format );
        depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp         = PGToVulkanLoadAction( desc.depthAttachmentDescriptor.loadAction );
        depthAttachment.storeOp        = PGToVulkanStoreAction( desc.depthAttachmentDescriptor.storeAction );
        depthAttachment.stencilLoadOp  = PGToVulkanLoadAction( LoadAction::DONT_CARE );
        depthAttachment.stencilStoreOp = PGToVulkanStoreAction( StoreAction::DONT_CARE );
        depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference depthAttachmentRef = {};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = static_cast< uint32_t >( attachmentRefs.size() );
        subpass.pColorAttachments    = attachmentRefs.data();
        if ( desc.depthAttachmentDescriptor.format != PixelFormat::INVALID )
        {
            subpass.pDepthStencilAttachment = &depthAttachmentRef;
            attachments.push_back( depthAttachment );
        }

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

        if ( vkCreateRenderPass( m_handle, &renderPassInfo, nullptr, &pass.m_handle ) != VK_SUCCESS )
        {
            pass.m_handle = VK_NULL_HANDLE;
        }

        return pass;
    }

    void Device::Copy( Buffer dst, Buffer src ) const
    {
        CommandBuffer cmdBuf = g_renderState.transientCommandPool.NewCommandBuffer();

        cmdBuf.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );
        cmdBuf.Copy( dst, src );
        cmdBuf.EndRecording();

        Submit( cmdBuf );
        WaitForIdle();

        cmdBuf.Free();
    }

    void Device::CopyBufferToImage( const Buffer& buffer, const Texture& tex ) const
    {
        CommandBuffer cmdBuf = g_renderState.transientCommandPool.NewCommandBuffer();
        cmdBuf.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );

        VkBufferImageCopy region               = {};
        region.bufferOffset                    = 0;
        region.bufferRowLength                 = 0;
        region.bufferImageHeight               = 0;
        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = 1;
        region.imageOffset                     = { 0, 0, 0 };
        region.imageExtent                     = { tex.GetWidth(), tex.GetHeight(), 1 };

        vkCmdCopyBufferToImage(
            cmdBuf.GetHandle(),
            buffer.GetHandle(),
            tex.GetHandle(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        cmdBuf.EndRecording();
        g_renderState.device.Submit( cmdBuf );
        g_renderState.device.WaitForIdle();
        cmdBuf.Free();
    }

    VkDevice Device::GetHandle() const
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
            vkCmdBufs[i] = cmdBufs[i].GetHandle();
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
        // vkDeviceWaitIdle( g_renderState.device.GetHandle() );
    }

} // namespace Gfx
} // namespace Progression
