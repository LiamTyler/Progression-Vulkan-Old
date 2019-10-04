#pragma once

#include "graphics/graphics_api/command_buffer.hpp"
#include "graphics/graphics_api/fence.hpp"

namespace Progression
{
namespace Gfx
{

    class Device
    {
    public:
        Device() = default;

        void Free();
        operator bool() const;

        static Device CreateDefault();
        CommandPool NewCommandPool() const;
        Fence NewFence() const;
        Buffer NewBuffer( size_t length, BufferType type ) const;
        void SubmitRenderCommands( int numBuffers, CommandBuffer* cmdBufs ) const;
        void SubmitFrame( uint32_t imageIndex ) const;

        VkDevice GetNativeHandle() const;
        VkQueue GraphicsQueue() const;
        VkQueue PresentQueue() const;

    private:
        VkDevice m_handle        = VK_NULL_HANDLE;
        VkQueue  m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue  m_presentQueue  = VK_NULL_HANDLE;
    };

} // namespace Gfx
} // namespace Progression