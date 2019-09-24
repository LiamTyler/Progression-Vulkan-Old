#include "graphics/render_system.hpp"
#include "core/assert.hpp"
#include "core/scene.hpp"
#include "core/window.hpp"
#include "graphics/graphics_api.hpp"
#include "resource/resource_manager.hpp"
#include "resource/shader.hpp"
#include "utils/logger.hpp"
#include <array>
#include <unordered_map>
#include "graphics/vulkan.hpp"

using namespace Progression;
using namespace Gfx;

static std::unordered_map< std::string, Gfx::Sampler > s_samplers;

namespace Progression
{

namespace RenderSystem
{

    Gfx::Sampler* AddSampler( const std::string& name, Gfx::SamplerDescriptor& desc )
    {
        auto it = s_samplers.find( name );
        if ( it != s_samplers.end() )
        {
            return &it->second;
        }
        else
        {
            s_samplers[name] = Gfx::Sampler::Create( desc );
            return &s_samplers[name];
        }
    }

    Gfx::Sampler* GetSampler( const std::string& name )
    {
        auto it = s_samplers.find( name );
        if ( it != s_samplers.end() )
        {
            return &it->second;
        }

        return nullptr;
    }

} // namespace RenderSystem
} // namespace Progression

static Window* s_window;
static Pipeline s_pipeline;

namespace Progression
{
namespace RenderSystem
{

    void InitSamplers();

    bool Init()
    {
        if ( !VulkanInit() )
        {
            LOG_ERR( "Could not initialize vulkan" );
            return false;
        }

        InitSamplers();
        s_window = GetMainWindow();

        PipelineDescriptor pipelineDesc;
        pipelineDesc.renderPass       = &g_renderState.renderPass;
        pipelineDesc.vertexDescriptor = VertexInputDescriptor::Create( 0, nullptr, 0, nullptr );

        pipelineDesc.viewport.x        = 0.0f;
        pipelineDesc.viewport.y        = 0.0f;
        pipelineDesc.viewport.width    = static_cast< float >( g_renderState.swapChain.extent.width );
        pipelineDesc.viewport.height   = static_cast< float >( g_renderState.swapChain.extent.height );
        pipelineDesc.viewport.minDepth = 0.0f;
        pipelineDesc.viewport.maxDepth = 1.0f;

        pipelineDesc.scissor.x      = 0;
        pipelineDesc.scissor.y      = 0;
        pipelineDesc.scissor.width  = g_renderState.swapChain.extent.width;
        pipelineDesc.scissor.height = g_renderState.swapChain.extent.height;

        ResourceManager::LoadFastFile( PG_RESOURCE_DIR "cache/fastfiles/resource.txt.ff" );
        auto simpleVert = ResourceManager::Get< Shader >( "simpleVert" );
        auto simpleFrag = ResourceManager::Get< Shader >( "simpleFrag" );
        pipelineDesc.shaders[0] = simpleVert.get();
        pipelineDesc.shaders[1] = simpleFrag.get();
        pipelineDesc.numShaders = 2;

        s_pipeline = Pipeline::Create( pipelineDesc );
        if ( !s_pipeline )
        {
            LOG_ERR( "Could not create pipeline" );
            return false;
        }

        for ( size_t i = 0; i < g_renderState.commandBuffers.size(); ++i )
        {
            const VkCommandBuffer& cmdBuf = g_renderState.commandBuffers[i];
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags            = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            beginInfo.pInheritanceInfo = nullptr;

            if ( vkBeginCommandBuffer( cmdBuf, &beginInfo ) != VK_SUCCESS )
            {
                return false;
            }

            // specify which render pass, which framebuffer, where shader loads start, and size
            VkRenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass        = g_renderState.renderPass.GetNativeHandle();
            renderPassInfo.framebuffer       = g_renderState.swapChainFramebuffers[i];
            renderPassInfo.renderArea.offset = { 0, 0 };
            renderPassInfo.renderArea.extent = g_renderState.swapChain.extent;

            VkClearValue clearColor        = { 0.0f, 0.0f, 0.0f, 1.0f };
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues    = &clearColor;

            vkCmdBeginRenderPass( cmdBuf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );
            vkCmdBindPipeline( cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, s_pipeline.GetNativeHandle() );
            vkCmdDraw( cmdBuf, 3, 1, 0, 0 );
            vkCmdEndRenderPass( cmdBuf );

            if ( vkEndCommandBuffer( cmdBuf ) != VK_SUCCESS )
            {
                return false;
            }
        }

        return true;
    }

    void Shutdown()
    {
        for ( auto& [name, sampler] : s_samplers )
        {
            sampler = {};
        }

        s_pipeline = {};

        VulkanShutdown();
    }

    void Render( Scene* scene )
    {
        PG_UNUSED( scene );

        VkDevice dev = g_renderState.device.GetNativeHandle();
        uint32_t imageIndex;
        vkAcquireNextImageKHR( dev, g_renderState.swapChain.swapChain, UINT64_MAX,
                               g_renderState.presentComplete, VK_NULL_HANDLE, &imageIndex );

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[]      = { g_renderState.presentComplete };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount     = 1;
        submitInfo.pWaitSemaphores        = waitSemaphores;
        submitInfo.pWaitDstStageMask      = waitStages;
        submitInfo.commandBufferCount     = 1;
        submitInfo.pCommandBuffers        = &g_renderState.commandBuffers[imageIndex];

        VkSemaphore signalSemaphores[]    = { g_renderState.renderComplete };
        submitInfo.signalSemaphoreCount   = 1;
        submitInfo.pSignalSemaphores      = signalSemaphores;

        PG_ASSERT( vkQueueSubmit( g_renderState.device.GraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE ) == VK_SUCCESS );

        VkPresentInfoKHR presentInfo   = {};
        presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = signalSemaphores;

        VkSwapchainKHR swapChains[] = { g_renderState.swapChain.swapChain };
        presentInfo.swapchainCount  = 1;
        presentInfo.pSwapchains     = swapChains;
        presentInfo.pImageIndices   = &imageIndex;

        vkQueuePresentKHR( g_renderState.device.PresentQueue(), &presentInfo);
        // if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        //     framebufferResized = false;
        //     recreateSwapChain();
        // } else if (result != VK_SUCCESS) {
        //     return false;
        // }

        // currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

        // // only needed if validation layers are on
        // vkDeviceWaitIdle(logicalDevice);

    }

    void InitSamplers()
    {
        SamplerDescriptor samplerDesc;

        samplerDesc.minFilter = FilterMode::NEAREST;
        samplerDesc.magFilter = FilterMode::NEAREST;
        samplerDesc.wrapModeS = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeT = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeR = WrapMode::CLAMP_TO_EDGE;
        //s_samplers["nearest"] = Sampler::Create( samplerDesc );

        samplerDesc.minFilter = FilterMode::LINEAR;
        samplerDesc.magFilter = FilterMode::LINEAR;
        //s_samplers["linear"] = Sampler::Create( samplerDesc );
    }


} // namespace RenderSystem
} // namespace Progression
