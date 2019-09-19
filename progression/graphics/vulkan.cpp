#include "graphics/vulkan.hpp"
#include "core/platform_defines.hpp"
#include "utils/logger.hpp"
#include <vulkan/vulkan.hpp>
#include "GLFW/glfw3.h"
#include <iostream>
#include <string>
#include <vector>

namespace Progression
{
namespace Gfx
{

// the standard layer enables a bunch of useful diagnostic layers
const std::vector<const char*> validationLayers = {
    "VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static VkInstance instance;


std::vector< std::string > FindMissingValidationLayers( const std::vector< const char* >& layers )
{
    LOG( "test" );
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
    appInfo.apiVersion         = VK_API_VERSION_1_0;

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
    LOG( "Vulkan extensions needed" );
    for ( size_t i = 0; i < extensionNames.size(); i++ )
    {
        LOG( "\t", extensionNames[i] );
    }

    // Specify global validation layers
#if !USING( SHIP_BUILD )
    createInfo.enabledLayerCount   = static_cast< uint32_t >( validationLayers.size() );
    createInfo.ppEnabledLayerNames = validationLayers.data();
#else // #if !USING( SHIP_BUILD )
    createInfo.enabledLayerCount   = 0;
#endif // #else // #if !USING( SHIP_BUILD )

    auto ret = vkCreateInstance( &createInfo, nullptr, &instance );
    if ( ret == VK_ERROR_EXTENSION_NOT_PRESENT )
    {
        LOG_ERR( "Could not find all of the instance extensions" );
    }
    else if ( ret == VK_ERROR_LAYER_NOT_PRESENT )
    {
        auto missingLayers = FindMissingValidationLayers( validationLayers );
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

bool VulkanInit()
{
    if ( !CreateInstance() )
    {
        LOG_ERR( "Could not create vulkan instance" );
        return false;
    }

    return true;
}

void VulkanShutdown()
{
    vkDestroyInstance( instance, nullptr );
}

} // namespace Gfx
} // namespace Progression
