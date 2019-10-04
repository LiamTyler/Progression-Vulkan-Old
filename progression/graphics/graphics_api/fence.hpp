#pragma once

#include <vulkan/vulkan.hpp>

namespace Progression
{
namespace Gfx
{

    class Fence
    {
        friend class Device;
    public:
        Fence() = default;

        void Free();
        void WaitFor();
        void Reset();

    private:
        VkFence m_handle  = VK_NULL_HANDLE;
        VkDevice m_device;
    };

} // namespace Gfx
} // namespace Progression