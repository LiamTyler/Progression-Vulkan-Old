#include "graphics/vulkan.hpp"
#include "core/platform_defines.hpp"
#include <vulkan/vulkan.hpp>
#include "core/window.hpp"
#include "graphics/graphics_api.hpp"
#include "graphics/pg_to_vulkan_types.hpp"
#include "utils/logger.hpp"
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace Progression
{
namespace Gfx
{

RenderState g_renderState;

static std::vector< std::string > FindMissingValidationLayers( const std::vector< const char* >& layers )
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties( &layerCount, nullptr );
    std::vector< VkLayerProperties > availableLayers( layerCount );
    vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() );

    LOG( "Available validation layers:" );
    for ( const auto& layerProperties : availableLayers )
    {
        LOG( "\t", layerProperties.layerName );
    }

    std::vector< std::string > missing;
    for ( const auto& layer : layers )
    {
        bool found = false;
        for ( const auto& availableLayer : availableLayers )
        {
            if ( strcmp( layer, availableLayer.layerName ) == 0 )
            {
                found = true;
                break;
            }
        }

        if ( !found )
        {
            missing.push_back( layer );
        }
    }

    return missing;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData )
{
    PG_UNUSED( pUserData );

    std::string messageTypeString;
    switch ( messageType )
    {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
            messageTypeString = "General";
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
            messageTypeString = "Validation";
            break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
            messageTypeString = "Performance";
            break;
        default:
            messageTypeString = "Unknown";
    }

    if ( messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
    {
        LOG_ERR( "Vulkan message type ", messageTypeString, ": ", pCallbackData->pMessage, "'" );
    }
    else if ( messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
    {
        LOG_WARN( "Vulkan message type ", messageTypeString, ": ", pCallbackData->pMessage, "'" );
    }
    else
    {
        LOG( "Vulkan message type ", messageTypeString, ": '", pCallbackData->pMessage, "'" );
    }

    return VK_FALSE;
}

/** Since the createDebugUtilsMessenger function is from an extension, it is not loaded
 * automatically. Look up its address manually and call it.
 */
static bool CreateDebugUtilsMessengerEXT(
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator )
{
    auto func = ( PFN_vkCreateDebugUtilsMessengerEXT ) vkGetInstanceProcAddr( g_renderState.instance, "vkCreateDebugUtilsMessengerEXT" );
    if ( func != nullptr )
    {
        return func( g_renderState.instance, pCreateInfo, pAllocator, &g_renderState.debugMessenger ) == VK_SUCCESS;
    }
    else
    {
        return false;
    }
}

/** Same thing as the CreateDebugUtilsMessenger; load the function manually and call it
 */
static void DestroyDebugUtilsMessengerEXT( const VkAllocationCallbacks* pAllocator = nullptr )
{
    auto func = ( PFN_vkDestroyDebugUtilsMessengerEXT ) vkGetInstanceProcAddr( g_renderState.instance, "vkDestroyDebugUtilsMessengerEXT" );
    if ( func != nullptr )
    {
        func( g_renderState.instance, g_renderState.debugMessenger, pAllocator );
    }
}

/** \brief Find and select the first avaiable queues for graphics and presentation
* Queues are where commands get submitted to and are processed asynchronously. Some queues
* might only be usable for certain operations, like graphics or memory operations.
* Currently we just need 1 queue for graphics commands, and 1 queue for
* presenting the images we create to the surface.
*/
static QueueFamilyIndices FindQueueFamilies( VkPhysicalDevice device, VkSurfaceKHR surface )
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, nullptr );

    std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
    vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies.data() );

    int i = 0;
    for ( const auto& queueFamily : queueFamilies )
    {
        // check if the queue supports graphics operations
        if ( queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
        {
            indices.graphicsFamily = i;
        }

        // check if the queue supports presenting images to the surface
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR( device, i, surface, &presentSupport );

        if ( queueFamily.queueCount > 0 && presentSupport )
        {
            indices.presentFamily = i;
        }

        if ( indices.IsComplete() )
        {
            break;
        }

        i++;
    }

    return indices;
}

static bool CheckPhysicalDeviceExtensionSupport( VkPhysicalDevice device )
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, nullptr );

    std::vector< VkExtensionProperties > availableExtensions( extensionCount );
    vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, availableExtensions.data() );

    std::set< std::string > requiredExtensions( VK_DEVICE_EXTENSIONS.begin(), VK_DEVICE_EXTENSIONS.end() );

    for ( const auto& extension : availableExtensions )
    {
        requiredExtensions.erase( extension.extensionName );
    }

    return requiredExtensions.empty();
}

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector< VkSurfaceFormatKHR > formats;
    std::vector< VkPresentModeKHR > presentModes;
};

static VkExtent2D ChooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities )
{
    // check if the driver specified the size already
    if ( capabilities.currentExtent.width != std::numeric_limits< uint32_t >::max() )
    {
        return capabilities.currentExtent;
    }
    else
    {
        // select the closest feasible resolution possible to the window size
        int SW = GetMainWindow()->Width();
        int SH = GetMainWindow()->Height();
        glfwGetFramebufferSize( GetMainWindow()->GetGLFWHandle(), &SW, &SH );

        VkExtent2D actualExtent =
        {
            static_cast< uint32_t >( SW ),
            static_cast< uint32_t >( SH )
        };

        actualExtent.width = std::max( capabilities.minImageExtent.width,
                                       std::min( capabilities.maxImageExtent.width, actualExtent.width ) );
        actualExtent.height = std::max( capabilities.minImageExtent.height,
                                        std::min( capabilities.maxImageExtent.height, actualExtent.height ) );

        return actualExtent;
    }
}

static VkPresentModeKHR ChooseSwapPresentMode( const std::vector< VkPresentModeKHR >& availablePresentModes )
{
    VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;

    for (const auto& availablePresentMode : availablePresentModes)
    {
        // if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
        if ( availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR )
        {
            LOG( "Found mailbox mode" );
            return availablePresentMode;
        }
        // else if ( availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR )
        // {
        //     mode = availablePresentMode;
        // }
    }

    LOG( "did not find mailbox mode" );
    return mode;
}

static VkSurfaceFormatKHR ChooseSwapSurfaceFormat( const std::vector< VkSurfaceFormatKHR >& availableFormats )
{
    // check if the surface has no preferred format (best case)
    // if ( availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED )
    // {
    //     return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    // }

    for ( const auto& availableFormat : availableFormats )
    {
        if ( availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
             availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
        {
            return availableFormat;
        }
    }

    LOG_WARN( "Format not RGBA8 with SRGB colorspace. Instead format = ",
              availableFormats[0].format, ", colorspace = ", availableFormats[0].colorSpace );

    return availableFormats[0];
}

static SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, surface, &details.capabilities );

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, nullptr );

    if ( formatCount != 0 )
    {
        details.formats.resize( formatCount );
        vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, details.formats.data() );
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, nullptr );

    if ( presentModeCount != 0 )
    {
        details.presentModes.resize( presentModeCount );
        vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, details.presentModes.data() );
    }

    return details;
}

/** Return a rating of how good this device is. 0 is incompatible, and the higher the better. */
static int RatePhysicalDevice( const PhysicalDeviceInfo& deviceInfo )
{
    // check the required features first: queueFamilies, extension support,
    // and swap chain support
    bool extensionsSupported = CheckPhysicalDeviceExtensionSupport( deviceInfo.device );

    bool swapChainAdequate = false;
    if ( extensionsSupported )
    {
        SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport( deviceInfo.device, g_renderState.surface );
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    LOG( "aniso supported: ", deviceInfo.deviceFeatures.samplerAnisotropy );
    if ( !deviceInfo.indices.IsComplete() || !extensionsSupported || !swapChainAdequate || !deviceInfo.deviceFeatures.samplerAnisotropy )
    {
        return 0;
    }

    int score = 10;
    if ( deviceInfo.deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
    {
        score += 1000;
    }
    
    return score;
}


static bool CreateInstance()
{
    // struct that holds info about our application. Mainly used by some layers / drivers
    // for labeling debug messages, logging, etc. Possible for drivers to run differently
    // depending on the application that is running.
    VkApplicationInfo appInfo  = {};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext              = nullptr; // pointer to extension information
    appInfo.pApplicationName   = "Progression";
    appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.pEngineName        = "Progression";
    appInfo.engineVersion      = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.apiVersion         = VK_API_VERSION_1_1;

    // non-optional struct that specifies which global extension and validation layers to use
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo     = &appInfo;

    // Vulkan by itself doesn't know how to do any platform specifc things, so we do need
    // extensions. Specifically, we at least need the ones to interface with the windowing API,
    // so ask glfw for the extensions needed for this. These are global to the program.
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );
    std::vector<const char*> extensionNames( glfwExtensions, glfwExtensions + glfwExtensionCount );

#if !USING( SHIP_BUILD )
    // Also want the debug utils extension so we can print out layer messages
    extensionNames.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
#endif // #if !USING( SHIP_BUILD )

    createInfo.enabledExtensionCount   = static_cast< uint32_t >( extensionNames.size() );
    createInfo.ppEnabledExtensionNames = extensionNames.data();

    // Specify global validation layers
#if !USING( SHIP_BUILD )
    createInfo.enabledLayerCount   = static_cast< uint32_t >( VK_VALIDATION_LAYERS.size() );
    createInfo.ppEnabledLayerNames = VK_VALIDATION_LAYERS.data();
#else // #if !USING( SHIP_BUILD )
    createInfo.enabledLayerCount   = 0;
#endif // #else // #if !USING( SHIP_BUILD )

    auto ret = vkCreateInstance( &createInfo, nullptr, &g_renderState.instance );
    if ( ret == VK_ERROR_EXTENSION_NOT_PRESENT )
    {
        LOG_ERR( "Could not find all of the instance extensions" );
    }
    else if ( ret == VK_ERROR_LAYER_NOT_PRESENT )
    {
        auto missingLayers = FindMissingValidationLayers( VK_VALIDATION_LAYERS );
        LOG_ERR( "Could not find the following validation layers: " );
        for ( const auto& layer : missingLayers )
        {
            LOG_ERR( "\t", layer );
        }
    }
    else if ( ret != VK_SUCCESS )
    {
        LOG_ERR( "Error while creating instance: ", ret );
    }

    return ret == VK_SUCCESS;
}

static bool SetupDebugCallback()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT; // general verbose debug info
    
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    createInfo.pfnUserCallback = DebugCallback;
    createInfo.pUserData = nullptr;

    return CreateDebugUtilsMessengerEXT( &createInfo, nullptr );
}

static bool CreateSurface()
{
    return glfwCreateWindowSurface( g_renderState.instance, GetMainWindow()->GetGLFWHandle(), nullptr, &g_renderState.surface ) == VK_SUCCESS;
}

static bool PickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices( g_renderState.instance, &deviceCount, nullptr );

    if ( deviceCount == 0 )
    {
        return false;
    }

    std::vector< VkPhysicalDevice > devices( deviceCount );
    vkEnumeratePhysicalDevices( g_renderState.instance, &deviceCount, devices.data() );

    std::vector< PhysicalDeviceInfo > deviceInfos( deviceCount );
    for ( uint32_t i = 0; i < deviceCount; ++i )
    {
        deviceInfos[i].device  = devices[i];
        vkGetPhysicalDeviceProperties( devices[i], &deviceInfos[i].deviceProperties );
        vkGetPhysicalDeviceFeatures( devices[i], &deviceInfos[i].deviceFeatures );
        deviceInfos[i].deviceFeatures.samplerAnisotropy = VK_TRUE; 
        deviceInfos[i].name    = deviceInfos[i].deviceProperties.deviceName;
        deviceInfos[i].indices = FindQueueFamilies( devices[i], g_renderState.surface );
        deviceInfos[i].score   = RatePhysicalDevice( deviceInfos[i] );
    }

    // sort and select the best GPU available
    std::sort( deviceInfos.begin(), deviceInfos.end(), []( const auto& lhs, const auto& rhs ) { return lhs.score > rhs.score; } );

    auto& physicalInfo = g_renderState.physicalDeviceInfo;
    physicalInfo       = deviceInfos[0];

    if ( physicalInfo.score <= 0 )
    {
        physicalInfo.device = VK_NULL_HANDLE;
        return false;
    }

    vkGetPhysicalDeviceMemoryProperties( physicalInfo.device, &physicalInfo.memProperties );
    
    return true;
}

static bool CreateRenderPass()
{
    RenderPassDescriptor renderPassDesc;
    renderPassDesc.colorAttachmentDescriptors[0].format = VulkanToPGPixelFormat( g_renderState.swapChain.imageFormat );
    g_renderState.renderPass = g_renderState.device.NewRenderPass( renderPassDesc );
    return g_renderState.renderPass;
}

static bool CreateSwapChainFrameBuffers()
{
    VkImageView attachments[1];

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = g_renderState.renderPass.GetHandle();
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments    = attachments;
    framebufferInfo.width           = g_renderState.swapChain.extent.width;
    framebufferInfo.height          = g_renderState.swapChain.extent.height;
    framebufferInfo.layers          = 1;

    g_renderState.swapChainFramebuffers.resize( g_renderState.swapChain.images.size() );
    for ( size_t i = 0; i < g_renderState.swapChain.images.size(); ++i )
    {
        attachments[0] = g_renderState.swapChain.imageViews[i];
        if ( vkCreateFramebuffer( g_renderState.device.GetHandle(), &framebufferInfo, nullptr, &g_renderState.swapChainFramebuffers[i] ) != VK_SUCCESS )
        {
            return false;
        }
    }

    return true;
}

static bool CreateCommandPoolAndBuffers()
{
    g_renderState.commandPool = g_renderState.device.NewCommandPool();
    if ( !g_renderState.commandPool )
    {
        return false;
    }

    g_renderState.commandBuffers.resize( g_renderState.swapChain.images.size() );

    for ( auto& cmdbuf : g_renderState.commandBuffers )
    {
        cmdbuf = g_renderState.commandPool.NewCommandBuffer();
        if ( !cmdbuf )
        {
            return false;
        }
    }

    return true;
}

static bool CreateSemaphores()
{
    g_renderState.renderCompleteSemaphores.resize( MAX_FRAMES_IN_FLIGHT );
    g_renderState.presentCompleteSemaphores.resize( MAX_FRAMES_IN_FLIGHT );
    g_renderState.inFlightFences.resize( MAX_FRAMES_IN_FLIGHT );

    VkSemaphoreCreateInfo semInfo = {};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkDevice dev = g_renderState.device.GetHandle();

    for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
    {
        if ( vkCreateSemaphore( dev, &semInfo, nullptr, &g_renderState.presentCompleteSemaphores[i] ) != VK_SUCCESS ||
             vkCreateSemaphore( dev, &semInfo, nullptr, &g_renderState.renderCompleteSemaphores[i] ) != VK_SUCCESS ||
             vkCreateFence( dev, &fenceInfo, nullptr, &g_renderState.inFlightFences[i] ) != VK_SUCCESS )
        {
            return false;
        }
    }

    return true;
}

bool SwapChain::Create( VkDevice dev )
{
    device = dev;
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport( g_renderState.physicalDeviceInfo.device, g_renderState.surface );

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat( swapChainSupport.formats );
    VkPresentModeKHR   presentMode   = ChooseSwapPresentMode( swapChainSupport.presentModes );

    imageFormat   = surfaceFormat.format;
    extent        = ChooseSwapExtent( swapChainSupport.capabilities );

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if ( swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount )
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface                  = g_renderState.surface;
    createInfo.minImageCount            = imageCount;
    createInfo.imageFormat              = surfaceFormat.format;
    createInfo.imageColorSpace          = surfaceFormat.colorSpace;
    createInfo.imageExtent              = extent;
    createInfo.imageArrayLayers         = 1; // always 1 unless doing VR
    createInfo.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const auto& indices = g_renderState.physicalDeviceInfo.indices;
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };

    if ( indices.graphicsFamily != indices.presentFamily )
    {
        LOG_WARN( "Graphics queue is not the same as the presentation queue! Possible performance drop" );
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    // can specify transforms to happen (90 rotation, horizontal flip, etc). None used for now
    createInfo.preTransform   = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode    = presentMode;
    createInfo.clipped        = VK_TRUE;

    // only applies if you have to create a new swap chain (like on window resizing)
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if ( vkCreateSwapchainKHR( device, &createInfo, nullptr, &swapChain ) != VK_SUCCESS )
    {
        return false;
    }

    vkGetSwapchainImagesKHR( device, swapChain, &imageCount, nullptr );
    images.resize( imageCount );
    vkGetSwapchainImagesKHR( device, swapChain, &imageCount, images.data() );

    // image views
    imageViews.resize( images.size() );

    for ( size_t i = 0; i < images.size(); ++i )
    {
        imageViews[i] = CreateImageView( images[i], imageFormat );
    }
    
    return true;
}

uint32_t SwapChain::AcquireNextImage( VkSemaphore presentCompleteSemaphore )
{
    vkAcquireNextImageKHR( device, swapChain, UINT64_MAX, presentCompleteSemaphore, VK_NULL_HANDLE, &currentImage );

    return currentImage;
}

bool VulkanInit()
{
    if ( !CreateInstance() )
    {
        LOG_ERR( "Could not create vulkan instance" );
        return false;
    }

#if !USING( SHIP_BUILD )
    if ( !SetupDebugCallback() )
    {
        LOG_ERR( "Could not setup the debug callback" );
        return false;
    }
#endif // #if !USING( SHIP_BUILD )

    if ( !CreateSurface() )
    {
        LOG_ERR( "Could not create glfw vulkan surface" );
        return false;
    }

    if ( !PickPhysicalDevice() )
    {
        LOG_ERR( "Could not find any suitable GPU device to use" );
        return false;
    }
    else
    {
        LOG( "Using device: ", g_renderState.physicalDeviceInfo.name );
        // uint32_t major = VK_VERSION_MAJOR( deviceProperties.apiVersion );
        // uint32_t minor = VK_VERSION_MINOR( deviceProperties.apiVersion );
        // uint32_t patch = VK_VERSION_PATCH( deviceProperties.apiVersion );
        // LOG( "Using Vulkan Version: ", major, ".", minor, ".", patch );
    }

    g_renderState.device = Device::CreateDefault();
    if ( !g_renderState.device )
    {
        LOG_ERR( "Could not create logical device" );
        return false;
    }

    if ( !g_renderState.swapChain.Create( g_renderState.device.GetHandle() ) )
    {
        LOG_ERR( "Could not create swap chain" );
        return false;
    }

    if ( !CreateRenderPass() )
    {
        LOG_ERR( "Could not create render pass" );
        return false;
    }

    if ( !CreateSwapChainFrameBuffers() )
    {
        LOG_ERR( "Could not create swap chain framebuffers" );
        return false;
    }

    if ( !CreateCommandPoolAndBuffers() )
    {
        LOG_ERR( "Could not create commandPool / buffers" );
        return false;
    }

    if ( !CreateSemaphores() )
    {
        LOG_ERR( "Could not create semaphores" );
        return false;
    }

    return true;
}

void VulkanShutdown()
{
    VkDevice dev = g_renderState.device.GetHandle();

    for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
    {
        vkDestroySemaphore( dev, g_renderState.presentCompleteSemaphores[i], nullptr );
        vkDestroySemaphore( dev, g_renderState.renderCompleteSemaphores[i], nullptr );
        vkDestroyFence( dev, g_renderState.inFlightFences[i], nullptr );
    }

    g_renderState.commandPool.Free();
    for ( auto framebuffer : g_renderState.swapChainFramebuffers )
    {
        vkDestroyFramebuffer( dev, framebuffer, nullptr );
    }

    g_renderState.renderPass.Free();

    for ( size_t i = 0; i < g_renderState.swapChain.imageViews.size(); ++i )
    {
        vkDestroyImageView( dev, g_renderState.swapChain.imageViews[i], nullptr );
    }

    vkDestroySwapchainKHR( dev, g_renderState.swapChain.swapChain, nullptr);
    g_renderState.device.Free();
    DestroyDebugUtilsMessengerEXT();
    vkDestroySurfaceKHR( g_renderState.instance, g_renderState.surface, nullptr );
    vkDestroyInstance( g_renderState.instance, nullptr );
}

uint32_t FindMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties )
{
    auto& memProperties = g_renderState.physicalDeviceInfo.memProperties;
    for ( uint32_t i = 0; i < memProperties.memoryTypeCount; ++i )
    {
        if ( ( typeFilter & ( 1 << i ) ) && ( memProperties.memoryTypes[i].propertyFlags & properties ) == properties )
        {
            return i;
        }
    }

    PG_ASSERT( false );

    return ~0u;
}

void TransitionImageLayout( VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout )
{
    CommandBuffer cmdBuf = g_renderState.transientCommandPool.NewCommandBuffer();
    cmdBuf.BeginRecording( COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT );

    VkImageMemoryBarrier barrier = {};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = oldLayout;
    barrier.newLayout                       = newLayout;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = image;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;
    barrier.srcAccessMask                   = 0;
    barrier.dstAccessMask                   = 0;
    PG_UNUSED( format );

    VkPipelineStageFlags srcStage, dstStage;
    if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if ( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        PG_ASSERT( false, "The transition barriers are unknown for the given old and new layouts" );
    }

    cmdBuf.PipelineBarrier( srcStage, dstStage, barrier );

    cmdBuf.EndRecording();
    g_renderState.device.Submit( cmdBuf );
    g_renderState.device.WaitForIdle();
    cmdBuf.Free();
}

bool FormatSupported( VkFormat format, VkFormatFeatureFlags requestedSupport )
{
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties( g_renderState.physicalDeviceInfo.device, format, &props );
    return ( props.optimalTilingFeatures & requestedSupport ) == requestedSupport;
}

VkImageView CreateImageView( VkImage image, VkFormat format )
{
    VkImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image        = image;
    viewCreateInfo.viewType     = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format       = format;
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    // specify image purpose and which part to access
    viewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    viewCreateInfo.subresourceRange.baseMipLevel   = 0;
    viewCreateInfo.subresourceRange.levelCount     = 1;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount     = 1;

    VkImageView view;
    VkResult res = vkCreateImageView( g_renderState.device.GetHandle(), &viewCreateInfo, nullptr, &view );
    PG_ASSERT( res == VK_SUCCESS );

    return view;
}

} // namespace Gfx
} // namespace Progression
