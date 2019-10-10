#pragma once

#include "graphics/graphics_api/render_pass.hpp"
#include "graphics/graphics_api/pipeline.hpp"
#include <vulkan/vulkan.hpp>

namespace Progression
{
namespace Gfx
{
    class DescriptorSet;

    enum CommandBufferUsageBits
    {
        COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT = 1 << 0,
    };

    typedef uint32_t CommandBufferUsage;

    class CommandBuffer
    {
        friend class CommandPool;
    public:
        CommandBuffer() = default;

        operator bool() const;
        VkCommandBuffer GetNativeHandle() const;

        void Free();
        bool BeginRecording( CommandBufferUsage flags = 0 ) const;
        bool EndRecording() const;
        void BeginRenderPass( const RenderPass& renderPass, VkFramebuffer framebuffer ) const;
        void EndRenderPass() const;
        void BindRenderPipeline( const Pipeline& pipeline ) const;
        void BindDescriptorSets( uint32_t numSets, DescriptorSet* sets, const Pipeline& pipeline, uint32_t firstSet = 0 ) const;
        void BindVertexBuffer( const Buffer& buffer, size_t offset = 0, uint32_t firstBinding = 0 ) const;
        void BindVertexBuffers( uint32_t numBuffers, const Buffer* buffers, size_t* offsets, uint32_t firstBinding = 0 ) const;
        void BindIndexBuffer( const Buffer& buffer, IndexType indexType, size_t offset = 0 ) const;

        void Copy( const Buffer& dst, const Buffer& src );

        void Draw( uint32_t firstVert, uint32_t vertCount, uint32_t instanceCount = 1, uint32_t firstInstance = 0 ) const;
        void DrawIndexed( uint32_t firstIndex, uint32_t indexCount, int vertexOffset = 0, uint32_t firstInstance = 0, uint32_t instanceCount = 1 ) const;

    private:
        VkDevice m_device        = VK_NULL_HANDLE;
        VkCommandPool m_pool     = VK_NULL_HANDLE;
        VkCommandBuffer m_handle = VK_NULL_HANDLE;
    };

    typedef enum CommandPoolCreateFlagBits
    {
        COMMAND_POOL_TRANSIENT = 1 << 0,
    } CommandPoolCreateFlagBits;

    typedef uint32_t CommandPoolCreateFlags;

    class CommandPool
    {
        friend class Device;
    public:
        CommandPool() = default;

        void Free();
        CommandBuffer NewCommandBuffer();
        operator bool() const;


    private:
        VkCommandPool m_handle = VK_NULL_HANDLE;
        VkDevice m_device      = VK_NULL_HANDLE;
    };

} // namespace Gfx
} // namespace Progression
