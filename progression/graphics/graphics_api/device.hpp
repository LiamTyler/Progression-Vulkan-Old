#pragma once

#include "graphics/graphics_api/command_buffer.hpp"
#include "graphics/graphics_api/descriptor.hpp"
#include "graphics/graphics_api/framebuffer.hpp"
#include "graphics/graphics_api/pipeline.hpp"
#include "graphics/graphics_api/texture.hpp"
#include "graphics/graphics_api/sampler.hpp"
#include "graphics/graphics_api/synchronization.hpp"

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

        void Submit( const CommandBuffer& cmdBuf ) const;
        void WaitForIdle() const;
        CommandPool NewCommandPool( CommandPoolCreateFlags flags = 0, CommandPoolQueueFamily family = CommandPoolQueueFamily::GRAPHICS, const std::string& name = "" ) const;
        DescriptorPool NewDescriptorPool( int numPoolSizes, VkDescriptorPoolSize* poolSizes, uint32_t maxSets = 1, const std::string& name = "" ) const;
        std::vector< DescriptorSetLayout > NewDescriptorSetLayouts( const std::vector< DescriptorSetLayoutData >& layoutData ) const;
        void UpdateDescriptorSets( uint32_t count, const VkWriteDescriptorSet* writes ) const;
        Buffer NewBuffer( size_t length, BufferType type, MemoryType memoryType, const std::string& name = "" ) const;
        Buffer NewBuffer( size_t length, void* data, BufferType type, MemoryType memoryType, const std::string& name = "" ) const;
        Texture NewTexture( const ImageDescriptor& desc, bool managed = true, const std::string& name = "" ) const;
        Texture NewTextureFromBuffer( ImageDescriptor& desc, void* data, bool managed = true, const std::string& name = "" ) const;
        Sampler NewSampler( const SamplerDescriptor& desc ) const;
        Fence NewFence( bool signaled, const std::string& name = "" ) const;
        Semaphore NewSemaphore( const std::string& name = "" ) const;
        Pipeline NewPipeline( const PipelineDescriptor& desc, const std::string& name = "" ) const;
        RenderPass NewRenderPass( const RenderPassDescriptor& desc, const std::string& name = "" ) const;
        Framebuffer NewFramebuffer( const std::vector< Texture* >& attachments, const RenderPass& renderPass, const std::string& name = "" ) const;
        Framebuffer NewFramebuffer( const VkFramebufferCreateInfo& info, const std::string& name = "" ) const;
        void SubmitRenderCommands( int numBuffers, CommandBuffer* cmdBufs ) const;
        void SubmitComputeCommand( const CommandBuffer& cmdBuf ) const;
        void SubmitFrame( uint32_t imageIndex ) const;

        void Copy( Buffer dst, Buffer src ) const;
        void CopyBufferToImage( const Buffer& buffer, const Texture& tex ) const;

        VkDevice GetHandle() const;
        VkQueue GraphicsQueue() const;
        VkQueue PresentQueue() const;

    private:
        VkDevice m_handle        = VK_NULL_HANDLE;
        VkQueue  m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue  m_presentQueue  = VK_NULL_HANDLE;
        VkQueue  m_computeQueue  = VK_NULL_HANDLE;
    };

} // namespace Gfx
} // namespace Progression
