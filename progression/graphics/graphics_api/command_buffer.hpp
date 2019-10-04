#pragma once

#include "graphics/graphics_api/render_pass.hpp"
#include "graphics/graphics_api/pipeline.hpp"
#include <vulkan/vulkan.hpp>

namespace Progression
{
namespace Gfx
{

    class CommandBuffer
    {
        friend class CommandPool;
    public:
        CommandBuffer() = default;

        operator bool() const;
        VkCommandBuffer GetNativeHandle() const;

        bool BeginRecording();
        bool EndRecording();
        void BeginRenderPass( const RenderPass& renderPass, VkFramebuffer framebuffer );
        void EndRenderPass();
        void BindRenderPipeline( const Pipeline& pipeline );

        void Draw( uint32_t firstVert, uint32_t vertCount, uint32_t instanceCount = 1, uint32_t firstInstance = 0 );
    private:
        VkCommandBuffer m_handle = VK_NULL_HANDLE;
        VkCommandBufferBeginInfo m_beginInfo;
    };

    class CommandPool
    {
        friend class Device;
    public:
        CommandPool() = default;

        void Free();
        operator bool() const;

        CommandBuffer NewCommandBuffer();

    private:
        VkCommandPool m_handle = VK_NULL_HANDLE;
        VkDevice m_device      = VK_NULL_HANDLE;
    };

} // namespace Gfx
} // namespace Progression