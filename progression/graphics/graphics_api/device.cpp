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
        pipelineLayoutInfo.setLayoutCount         = 0;
        pipelineLayoutInfo.pSetLayouts            = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges    = nullptr;

        if ( vkCreatePipelineLayout( m_handle, &pipelineLayoutInfo, nullptr, &p.m_pipelineLayout ) != VK_SUCCESS )
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
            colorAttachmentRefs[i].attachment = static_cast< uint32_t>( i );
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

        if ( vkCreateRenderPass( m_handle, &renderPassInfo, nullptr, &pass.m_handle ) != VK_SUCCESS )
        {
            pass.m_handle = VK_NULL_HANDLE;
        }

        return pass;
    }

    void Device::Copy( Buffer dst, Buffer src ) const
    {
        CommandBuffer buffer = g_renderState.transientCommandPool.NewCommandBuffer();

        buffer.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );
        buffer.Copy( dst, src );
        buffer.EndRecording();

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        VkCommandBuffer vkCmdBuf = buffer.GetNativeHandle();
        submitInfo.pCommandBuffers = &vkCmdBuf;

        vkQueueSubmit( m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle( m_graphicsQueue );

        buffer.Free();
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

} // namespace Gfx
} // namespace Progression
