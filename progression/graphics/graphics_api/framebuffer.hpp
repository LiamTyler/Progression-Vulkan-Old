#pragma once

#include "graphics/graphics_api/texture.hpp"
#include "core/math.hpp"
#include <array>
#include <vulkan/vulkan.h>

namespace Progression
{
namespace Gfx
{

    class Framebuffer
    {
        friend class Device;
    public:
        Framebuffer() = default;

        void Free();
        VkFramebuffer GetHandle() const;
        operator bool() const;

    private:
        VkFramebuffer m_handle = VK_NULL_HANDLE;
        VkDevice      m_device = VK_NULL_HANDLE;
    };

} // namespace Gfx
} // namespace Progression
