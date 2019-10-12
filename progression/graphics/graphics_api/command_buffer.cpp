#include "graphics/graphics_api/command_buffer.hpp"
#include "core/assert.hpp"
#include "graphics/graphics_api/descriptor.hpp"
#include "graphics/pg_to_vulkan_types.hpp"
#include "graphics/vulkan.hpp"

namespace Progression
{
namespace Gfx
{
    
    CommandBuffer::operator bool() const
    {
        return m_handle != VK_NULL_HANDLE;
    }
    
    VkCommandBuffer CommandBuffer::GetHandle() const
    {
        return m_handle;
    }

    void CommandBuffer::Free()
    {
        PG_ASSERT( m_handle != VK_NULL_HANDLE );
        vkFreeCommandBuffers( m_device, m_pool, 1, &m_handle );
        m_handle = VK_NULL_HANDLE;
    }

    bool CommandBuffer::BeginRecording( CommandBufferUsage flags ) const
    {
        PG_ASSERT( m_handle != VK_NULL_HANDLE );
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags            = PGToVulkanCommandBufferUsage( flags );
        beginInfo.pInheritanceInfo = nullptr;
        return vkBeginCommandBuffer( m_handle, &beginInfo ) == VK_SUCCESS;
    }

    bool CommandBuffer::EndRecording() const
    {
        return vkEndCommandBuffer( m_handle ) == VK_SUCCESS;
    }

    void CommandBuffer::BeginRenderPass( const RenderPass& renderPass, VkFramebuffer framebuffer ) const
    {
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass        = renderPass.GetHandle();
        renderPassInfo.framebuffer       = framebuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = g_renderState.swapChain.extent;

        const auto& col = renderPass.desc.colorAttachmentDescriptors[0].clearColor;
        VkClearValue clearColor        = { col.r, col.g, col.b, col.a };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues    = &clearColor;

        vkCmdBeginRenderPass( m_handle, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );
    }

    void CommandBuffer::EndRenderPass() const
    {
        vkCmdEndRenderPass( m_handle );
    }

    void CommandBuffer::BindRenderPipeline( const Pipeline& pipeline ) const
    {
        vkCmdBindPipeline( m_handle, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetHandle() );
    }

    void CommandBuffer::BindDescriptorSets( uint32_t numSets, DescriptorSet* sets, const Pipeline& pipeline, uint32_t firstSet ) const
    {
        vkCmdBindDescriptorSets( m_handle, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 pipeline.GetLayoutHandle(), firstSet, numSets, (VkDescriptorSet*) sets, 0, nullptr );
    }

    void CommandBuffer::BindVertexBuffer( const Buffer& buffer, size_t offset, uint32_t firstBinding ) const
    {
        VkBuffer vertexBuffers[] = { buffer.GetHandle() };
        vkCmdBindVertexBuffers( m_handle, firstBinding, 1, vertexBuffers, &offset );
    }

    void CommandBuffer::BindVertexBuffers( uint32_t numBuffers, const Buffer* buffers, size_t* offsets, uint32_t firstBinding ) const
    {
        std::vector< VkBuffer > vertexBuffers( numBuffers );
        for ( uint32_t i = 0; i < numBuffers; ++i )
        {
            vertexBuffers[i] = buffers[i].GetHandle();
        }

        vkCmdBindVertexBuffers( m_handle, firstBinding, numBuffers, vertexBuffers.data(), offsets );
    }

    void CommandBuffer::BindIndexBuffer( const Buffer& buffer, IndexType indexType, size_t offset ) const
    {
        vkCmdBindIndexBuffer( m_handle, buffer.GetHandle(), offset, PGToVulkanIndexType( indexType ) );
    }

    void CommandBuffer::PipelineBarrier( VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                                         const VkImageMemoryBarrier& barrier ) const
    {
        vkCmdPipelineBarrier( m_handle, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier );
    }

    void CommandBuffer::Copy( const Buffer& dst, const Buffer& src )
    {
        VkBufferCopy copyRegion = {};
        copyRegion.size = src.GetLength();
        vkCmdCopyBuffer( m_handle, src.GetHandle(), dst.GetHandle(), 1, &copyRegion );
    }
    
    void CommandBuffer::Draw( uint32_t firstVert, uint32_t vertCount, uint32_t instanceCount, uint32_t firstInstance ) const
    {
        vkCmdDraw( m_handle, vertCount, instanceCount, firstVert, firstInstance );
    }

    void CommandBuffer::DrawIndexed( uint32_t firstIndex, uint32_t indexCount, int vertexOffset, uint32_t firstInstance, uint32_t instanceCount ) const
    {
        vkCmdDrawIndexed( m_handle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
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
        buf.m_device = m_device;
        buf.m_pool   = m_handle;

        return buf;
    }

} // namespace Gfx
} // namespace Progression
