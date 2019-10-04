#pragma once

#include "graphics/graphics_api/render_pass.hpp"
#include "core/enum_bit_operations.hpp"
#include "graphics/graphics_api/pipeline.hpp"
#include <vulkan/vulkan.hpp>

namespace Progression
{
namespace Gfx
{

    enum class CommandBufferUsage
    {
        NONE            = 0,
        ONE_TIME_SUBMIT = 1 << 0,
    };
    DEFINE_ENUM_BITWISE_OPERATORS( CommandBufferUsage );

    class CommandBuffer
    {
        friend class CommandPool;
    public:
        CommandBuffer() = default;

        operator bool() const;
        VkCommandBuffer GetNativeHandle() const;

        void Free();
        bool BeginRecording( CommandBufferUsage flags = CommandBufferUsage::NONE );
        bool EndRecording();
        void BeginRenderPass( const RenderPass& renderPass, VkFramebuffer framebuffer );
        void EndRenderPass();
        void BindRenderPipeline( const Pipeline& pipeline );

        void Copy( const Buffer& dst, const Buffer& src );

        void Draw( uint32_t firstVert, uint32_t vertCount, uint32_t instanceCount = 1, uint32_t firstInstance = 0 );
    private:
        VkDevice m_device        = VK_NULL_HANDLE;
        VkCommandPool m_pool     = VK_NULL_HANDLE;
        VkCommandBuffer m_handle = VK_NULL_HANDLE;
        VkCommandBufferBeginInfo m_beginInfo;
    };

    enum class CommandPoolFlags
    {
        NONE      = 0,
        TRANSIENT = 1 << 0,
    };
    DEFINE_ENUM_BITWISE_OPERATORS( CommandPoolFlags );

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