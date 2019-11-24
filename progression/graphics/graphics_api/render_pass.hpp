#pragma once

#include "graphics/graphics_api/texture.hpp"
#include "glm/vec4.hpp"
#include <vulkan/vulkan.hpp>

namespace Progression
{
namespace Gfx
{

    enum class LoadAction
    {
        LOAD      = 0,
        CLEAR     = 1,
        DONT_CARE = 2,

        NUM_LOAD_ACTION
    };

    enum class StoreAction
    {
        STORE     = 0,
        DONT_CARE = 1,

        NUM_STORE_ACTION
    };

    class ColorAttachmentDescriptor
    {
    public:
        ColorAttachmentDescriptor() = default;

        glm::vec4 clearColor    = glm::vec4( 0 );
        LoadAction loadAction   = LoadAction::CLEAR;
        StoreAction storeAction = StoreAction::STORE;
        PixelFormat format      = PixelFormat::INVALID;
    };

    class DepthAttachmentDescriptor
    {
    public:
        DepthAttachmentDescriptor() = default;

        float clearValue        = 1.0f;
        LoadAction loadAction   = LoadAction::CLEAR;
        StoreAction storeAction = StoreAction::STORE;
        PixelFormat format      = PixelFormat::INVALID;
    };

    class RenderPassDescriptor
    {
    friend class RenderPass;
    public:
        RenderPassDescriptor() = default;

        std::array< ColorAttachmentDescriptor, 8 > colorAttachmentDescriptors;
        DepthAttachmentDescriptor depthAttachmentDescriptor;
    };

    class RenderPass
    {
        friend class Device;
    public:
        RenderPass() = default;

        void Free();
        VkRenderPass GetHandle() const;
        operator bool() const;

        RenderPassDescriptor desc;

    // private:
        VkRenderPass m_handle = VK_NULL_HANDLE;
        VkDevice     m_device = VK_NULL_HANDLE;
    };

} // namespace Gfx
} // namespace Progression
