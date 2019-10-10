#pragma once

#include "graphics/graphics_api/command_buffer.hpp"
#include "graphics/graphics_api/descriptor.hpp"
#include "graphics/graphics_api/fence.hpp"
#include "graphics/graphics_api/pipeline.hpp"

namespace Progression
{
namespace Gfx
{

    class Device
    {
    public:
        Device() = default;

        static Device CreateDefault();
        void Free();
        operator bool() const;

        CommandPool NewCommandPool( CommandPoolCreateFlags flags = 0 ) const;
        DescriptorPool NewDescriptorPool( int numPoolSizes, VkDescriptorPoolSize* poolSizes, uint32_t maxSets = 1 ) const;
        Buffer NewBuffer( size_t length, BufferType type, MemoryType memoryType ) const;
        Buffer NewBuffer( size_t length, void* data, BufferType type, MemoryType memoryType ) const;
        Fence NewFence() const;
        Pipeline NewPipeline( const PipelineDescriptor& desc ) const;
        RenderPass NewRenderPass( const RenderPassDescriptor& desc ) const;
        void SubmitRenderCommands( int numBuffers, CommandBuffer* cmdBufs ) const;
        void SubmitFrame( uint32_t imageIndex ) const;

        void Copy( Buffer dst, Buffer src ) const;

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
