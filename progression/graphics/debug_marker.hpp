#pragma once

#include "core/math.hpp"
#include "core/platform_defines.hpp"
#include "graphics/vulkan.hpp"
#include <string>

namespace Progression
{
namespace Gfx
{

// From https://www.saschawillems.de/blog/2016/05/28/tutorial-on-using-vulkans-vk_ext_debug_marker-with-renderdoc/
namespace DebugMarker
{
	// Get function pointers for the debug report extensions from the device
	void Init( VkDevice device, VkPhysicalDevice physicalDevice );

    bool IsActive();

	// Sets the debug name of an object
	// All Objects in Vulkan are represented by their 64-bit handles which are passed into this function
	// along with the object type
	void SetObjectName( VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, const char *name );

	// Set the tag for an object
	void SetObjectTag( VkDevice device, uint64_t object, VkDebugReportObjectTypeEXT objectType, uint64_t name, size_t tagSize, const void* tag );

	// Start a new debug marker region
	void BeginRegion( VkCommandBuffer cmdbuffer, const char* pMarkerName, glm::vec4 color = glm::vec4( 0 ) );

	// Insert a new debug marker into the command buffer
	void Insert( VkCommandBuffer cmdbuffer, const char* name, glm::vec4 color = glm::vec4( 0 ) );

	// End the current debug marker region
	void EndRegion( VkCommandBuffer cmdBuffer );

    // Object specific naming functions
	void SetCommandPoolName( VkDevice device, VkCommandPool pool, const char * name );
	void SetCommandBufferName( VkDevice device, VkCommandBuffer cmdBuffer, const char * name );
	void SetQueueName( VkDevice device, VkQueue queue, const char * name );
	void SetImageName( VkDevice device, VkImage image, const char * name );
	void SetImageViewName( VkDevice device, VkImageView image, const char * name );
	void SetSamplerName( VkDevice device, VkSampler sampler, const char * name );
	void SetBufferName( VkDevice device, VkBuffer buffer, const char * name );
	void SetDeviceMemoryName( VkDevice device, VkDeviceMemory memory, const char * name );
	void SetShaderModuleName( VkDevice device, VkShaderModule shaderModule, const char * name );
	void SetPipelineName( VkDevice device, VkPipeline pipeline, const char * name );
	void SetPipelineLayoutName( VkDevice device, VkPipelineLayout pipelineLayout, const char * name );
	void SetRenderPassName( VkDevice device, VkRenderPass renderPass, const char * name );
	void SetFramebufferName( VkDevice device, VkFramebuffer framebuffer, const char * name );
	void SetDescriptorSetLayoutName( VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const char * name );
	void SetDescriptorSetName( VkDevice device, VkDescriptorSet descriptorSet, const char * name );
	void SetSemaphoreName( VkDevice device, VkSemaphore semaphore, const char * name );
	void SetFenceName( VkDevice device, VkFence fence, const char * name );
	void SetEventName( VkDevice device, VkEvent _event, const char * name );
	void SetSwapChainName( VkDevice device, VkSwapchainKHR swapchain, const char * name );
	// void SetPhysicalDeviceName( VkDevice device, VkPhysicalDevice pDev, const char * name ); // stops application in renderdoc?
	void SetLogicalDeviceName( VkDevice device, const char * name );
	void SetInstanceName( VkDevice device, VkInstance instance, const char * name );
	void SetDescriptorPoolName( VkDevice device, VkDescriptorPool pool, const char * name );

} // namespace DebugMarker
} // namespace Gfx
} // namespace Progression

#if !USING( SHIP_BUILD )

#define PG_DEBUG_MARKER_NAME( x, y ) ( std::string( x ) + y ).c_str()
#define PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( s, x ) if ( !s.empty() ) { x; }

#define PG_DEBUG_MARKER_BEGIN_REGION( cmdbuf, name, color ) Progression::Gfx::DebugMarker::BeginRegion( cmdbuf.GetHandle(), PG_DEBUG_MARKER_NAME( "", name ), color );
#define PG_DEBUG_MARKER_END_REGION( cmdbuf )                Progression::Gfx::DebugMarker::EndRegion( cmdbuf.GetHandle() );
#define PG_DEBUG_MARKER_INSERT( cmdbuf, name, color )       Progression::Gfx::DebugMarker::Insert( cmdbuf.GetHandle(), PG_DEBUG_MARKER_NAME( "", name ), color );

#define PG_DEBUG_MARKER_SET_BUFFER_NAME( buffer, name ) \
    Progression::Gfx::DebugMarker::SetBufferName( Progression::Gfx::g_renderState.device.GetHandle(), (buffer).GetHandle(), PG_DEBUG_MARKER_NAME( "Buffer: ", name ) ); \
    Progression::Gfx::DebugMarker::SetDeviceMemoryName( Progression::Gfx::g_renderState.device.GetHandle(), (buffer).GetMemoryHandle(), PG_DEBUG_MARKER_NAME( "Memory: buffer ", name ) )

#define PG_DEBUG_MARKER_SET_IMAGE_NAME( image, name ) \
    Progression::Gfx::DebugMarker::SetImageName( Progression::Gfx::g_renderState.device.GetHandle(), image.GetHandle(), PG_DEBUG_MARKER_NAME( "Image: ", name ) ); \
    Progression::Gfx::DebugMarker::SetImageViewName( Progression::Gfx::g_renderState.device.GetHandle(), image.GetView(), PG_DEBUG_MARKER_NAME( "Image View: ", name ) ); \
    Progression::Gfx::DebugMarker::SetDeviceMemoryName( Progression::Gfx::g_renderState.device.GetHandle(), image.GetMemoryHandle(), PG_DEBUG_MARKER_NAME( "Memory: image ", name ) )

#define PG_DEBUG_MARKER_SET_PIPELINE_NAME( pipeline, name ) \
    Progression::Gfx::DebugMarker::SetPipelineName( Progression::Gfx::g_renderState.device.GetHandle(), pipeline.GetHandle(), PG_DEBUG_MARKER_NAME( "Pipeline: ", name ) ); \
    Progression::Gfx::DebugMarker::SetPipelineLayoutName( Progression::Gfx::g_renderState.device.GetHandle(), pipeline.GetLayoutHandle(), PG_DEBUG_MARKER_NAME( "PipelineLayout: ", name ) )

#define PG_DEBUG_MARKER_SET_COMMAND_POOL_NAME( pool, name )       Progression::Gfx::DebugMarker::SetCommandPoolName( Progression::Gfx::g_renderState.device.GetHandle(), pool.GetHandle(), PG_DEBUG_MARKER_NAME( "Command Pool: ", name ) )
#define PG_DEBUG_MARKER_SET_COMMAND_BUFFER_NAME( cmdbuf, name )   Progression::Gfx::DebugMarker::SetCommandBufferName( Progression::Gfx::g_renderState.device.GetHandle(), cmdbuf.GetHandle(), PG_DEBUG_MARKER_NAME( "Command Buffer: ", name ) )
#define PG_DEBUG_MARKER_SET_QUEUE_NAME( queue, name )             Progression::Gfx::DebugMarker::SetQueueName( Progression::Gfx::g_renderState.device.GetHandle(), queue, PG_DEBUG_MARKER_NAME( "Queue: ", name ) )
#define PG_DEBUG_MARKER_SET_IMAGE_VIEW_NAME( view, name )         Progression::Gfx::DebugMarker::SetImageViewName( Progression::Gfx::g_renderState.device.GetHandle(), view, PG_DEBUG_MARKER_NAME( "Image View: ", name ) )
#define PG_DEBUG_MARKER_SET_IMAGE_ONLY_NAME( img, name )          Progression::Gfx::DebugMarker::SetImageName( Progression::Gfx::g_renderState.device.GetHandle(), img, PG_DEBUG_MARKER_NAME( "Image: ", name ) )
#define PG_DEBUG_MARKER_SET_SAMPLER_NAME( sampler, name )         Progression::Gfx::DebugMarker::SetSamplerName( Progression::Gfx::g_renderState.device.GetHandle(), sampler.GetHandle(), PG_DEBUG_MARKER_NAME( "Sampler: ", name ) )
#define PG_DEBUG_MARKER_SET_MEMORY_NAME( memory, name )           Progression::Gfx::DebugMarker::SetDeviceMemoryName( Progression::Gfx::g_renderState.device.GetHandle(), memory, PG_DEBUG_MARKER_NAME( "Memory: ", name ) )
#define PG_DEBUG_MARKER_SET_SHADER_NAME( shader, name )           Progression::Gfx::DebugMarker::SetShaderModuleName( Progression::Gfx::g_renderState.device.GetHandle(), shader.GetHandle(), PG_DEBUG_MARKER_NAME( "Shader: ", name ) )
#define PG_DEBUG_MARKER_SET_RENDER_PASS_NAME( pass, name )        Progression::Gfx::DebugMarker::SetRenderPassName( Progression::Gfx::g_renderState.device.GetHandle(), pass.GetHandle(), PG_DEBUG_MARKER_NAME( "RenderPass: ", name ) )
#define PG_DEBUG_MARKER_SET_FRAMEBUFFER_NAME( framebuffer, name ) Progression::Gfx::DebugMarker::SetFramebufferName( Progression::Gfx::g_renderState.device.GetHandle(), framebuffer, PG_DEBUG_MARKER_NAME( "Framebuffer: ", name ) )
#define PG_DEBUG_MARKER_SET_DESC_SET_LAYOUT_NAME( layout, name )  Progression::Gfx::DebugMarker::SetDescriptorSetLayoutName( Progression::Gfx::g_renderState.device.GetHandle(), layout.GetHandle(), PG_DEBUG_MARKER_NAME( "Descriptor Set Layout: ", name ) )
#define PG_DEBUG_MARKER_SET_DESC_SET_NAME( set, name )            Progression::Gfx::DebugMarker::SetDescriptorSetName( Progression::Gfx::g_renderState.device.GetHandle(), set.GetHandle(), PG_DEBUG_MARKER_NAME( "Descriptor Set: ", name ) )
#define PG_DEBUG_MARKER_SET_SEMAPHORE_NAME( semaphore, name )     Progression::Gfx::DebugMarker::SetSemaphoreName( Progression::Gfx::g_renderState.device.GetHandle(), semaphore.GetHandle(), PG_DEBUG_MARKER_NAME( "Semaphore: ", name ) )
#define PG_DEBUG_MARKER_SET_FENCE_NAME( fence, name )             Progression::Gfx::DebugMarker::SetFenceName( Progression::Gfx::g_renderState.device.GetHandle(), fence.GetHandle(), PG_DEBUG_MARKER_NAME( "Fence: ", name ) )
#define PG_DEBUG_MARKER_SET_SWAPCHAIN_NAME( swapchain, name )     Progression::Gfx::DebugMarker::SetSwapChainName( Progression::Gfx::g_renderState.device.GetHandle(), swapchain, PG_DEBUG_MARKER_NAME( "Swapchain: ", name ) )
// #define PG_DEBUG_MARKER_SET_PHYSICAL_DEVICE_NAME( pDev, name )    Progression::Gfx::DebugMarker::SetPhysicalDeviceName( Progression::Gfx::g_renderState.device.GetHandle(), pDev, PG_DEBUG_MARKER_NAME( "Physical Device: ", name ) )
#define PG_DEBUG_MARKER_SET_LOGICAL_DEVICE_NAME( dev, name )      Progression::Gfx::DebugMarker::SetLogicalDeviceName( dev.GetHandle(), PG_DEBUG_MARKER_NAME( "Device: ", name ) )
#define PG_DEBUG_MARKER_SET_INSTANCE_NAME( instance, name )       Progression::Gfx::DebugMarker::SetInstanceName( Progression::Gfx::g_renderState.device.GetHandle(), instance, PG_DEBUG_MARKER_NAME( "Instance: ", name ) )
#define PG_DEBUG_MARKER_SET_DESC_POOL_NAME( pool, name )          Progression::Gfx::DebugMarker::SetDescriptorPoolName( Progression::Gfx::g_renderState.device.GetHandle(), pool.GetHandle(), PG_DEBUG_MARKER_NAME( "Descriptor Pool: ", name ) )

#else // #if !USING( SHIP_BUILD )

#define PG_DEBUG_MARKER_NAME( x, y )
#define PG_DEBUG_MARKER_IF_STR_NOT_EMPTY( s, x )

#define PG_DEBUG_MARKER_BEGIN_REGION( cmdbuf, name, color )
#define PG_DEBUG_MARKER_END_REGION( cmdbuf )
#define PG_DEBUG_MARKER_INSERT( cmdbuf, name, color )

#define PG_DEBUG_MARKER_SET_COMMAND_POOL_NAME( pool, name )
#define PG_DEBUG_MARKER_SET_COMMAND_BUFFER_NAME( cmdbuf, name )
#define PG_DEBUG_MARKER_SET_QUEUE_NAME( queue, name )
#define PG_DEBUG_MARKER_SET_IMAGE_NAME( image, name )
#define PG_DEBUG_MARKER_SET_IMAGE_VIEW_NAME( view, name )
#define PG_DEBUG_MARKER_SET_IMAGE_ONLY_NAME( img, name )
#define PG_DEBUG_MARKER_SET_SAMPLER_NAME( sampler, name )
#define PG_DEBUG_MARKER_SET_BUFFER_NAME( buffer, name )
#define PG_DEBUG_MARKER_SET_MEMORY_NAME( memory, name )
#define PG_DEBUG_MARKER_SET_SHADER_NAME( shader, name )
#define PG_DEBUG_MARKER_SET_PIPELINE_NAME( pipeline, name )
#define PG_DEBUG_MARKER_SET_RENDER_PASS_NAME( pass, name )
#define PG_DEBUG_MARKER_SET_FRAMEBUFFER_NAME( framebuffer, name )
#define PG_DEBUG_MARKER_SET_DESC_SET_LAYOUT_NAME( layout, name )
#define PG_DEBUG_MARKER_SET_DESC_SET_NAME( set, name )
#define PG_DEBUG_MARKER_SET_SEMAPHORE_NAME( semaphore, name )
#define PG_DEBUG_MARKER_SET_FENCE_NAME( fence, name )
#define PG_DEBUG_MARKER_SET_SWAPCHAIN_NAME( swapchain, name )
// #define PG_DEBUG_MARKER_SET_PHYSICAL_DEVICE_NAME( pDev, name )
#define PG_DEBUG_MARKER_SET_LOGICAL_DEVICE_NAME( dev, name )
#define PG_DEBUG_MARKER_SET_INSTANCE_NAME( instance, name )
#define PG_DEBUG_MARKER_SET_DESC_POOL_NAME( pool, name )

#endif // #else // #if !USING( SHIP_BUILD )
