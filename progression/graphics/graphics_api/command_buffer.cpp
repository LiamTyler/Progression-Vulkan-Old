#include "graphics/graphics_api/command_buffer.hpp"
#include "core/assert.hpp"
#include "graphics/vulkan.hpp"

namespace Progression
{
namespace Gfx
{
    
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