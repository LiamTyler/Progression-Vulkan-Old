#pragma once

#include "graphics/graphics_api.hpp"
#include <vector>
#include <vulkan/vulkan.hpp>

namespace Progression
{
namespace Gfx
{
    const int MAX_FRAMES_IN_FLIGHT = 2;

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
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        std::string name;
        int score;
        QueueFamilyIndices indices;
    };

    struct SwapChain
    {
        VkSwapchainKHR swapChain;
        VkFormat imageFormat;
        VkExtent2D extent;
        std::vector< VkImage > images;
        std::vector< VkImageView > imageViews;
    };

    struct RenderState
    {
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;
        PhysicalDeviceInfo physicalDeviceInfo;
        SwapChain swapChain;
        std::vector< VkFramebuffer > swapChainFramebuffers;
        // VkCommandPool commandPool;
        // std::vector< VkCommandBuffer > commandBuffers;
        CommandPool commandPool;
        std::vector< CommandBuffer > commandBuffers;
        
        std::vector< VkSemaphore > presentCompleteSemaphores;
        std::vector< VkSemaphore > renderCompleteSemaphores;
        std::vector< VkFence > inFlightFences;

        Device device;
        RenderPass renderPass;
        size_t currentFrame = 0;
    };

    extern RenderState g_renderState;

    bool VulkanInit();

    void VulkanShutdown();

} // namepspace Gfx
} // namepspace Progression
