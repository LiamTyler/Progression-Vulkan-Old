#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

namespace Progression
{
namespace Gfx
{

    const std::vector< const char* > VK_VALIDATION_LAYERS =
    {
        "VK_LAYER_LUNARG_standard_validation"
    };

    const std::vector< const char* > VK_DEVICE_EXTENSIONS =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    struct QueueFamilyIndices
    {
        uint32_t graphicsFamily = ~0u;
        uint32_t presentFamily  = ~0u;

        bool IsComplete() const
        {
            return graphicsFamily != ~0u && presentFamily != ~0u;
        }
    };

    struct PhysicalDeviceInfo
    {
        VkPhysicalDevice device;
        std::string name;
        int score;
        QueueFamilyIndices indices;
    };

    PhysicalDeviceInfo* GetPhysicalDeviceInfo();

    bool VulkanInit();

    void VulkanShutdown();

} // namepspace Gfx
} // namepspace Progression
