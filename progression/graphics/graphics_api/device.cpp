#include "graphics/graphics_api/device.hpp"
#include "core/assert.hpp"
#include "core/platform_defines.hpp"
#include "graphics/pg_to_vulkan_types.hpp"
#include "graphics/vulkan.hpp"
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

} // namespace Gfx
} // namespace Progression