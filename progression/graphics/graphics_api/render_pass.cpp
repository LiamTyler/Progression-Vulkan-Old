#include "graphics/graphics_api/render_pass.hpp"
#include "graphics/pg_to_vulkan_types.hpp"
#include "graphics/vulkan.hpp"

namespace Progression
{
namespace Gfx
{

    RenderPass RenderPass::Create( const RenderPassDescriptor& desc )
    {
        RenderPass pass;
        pass.desc = desc;

        std::vector< VkAttachmentDescription > colorAttachments;
        std::vector< VkAttachmentReference > colorAttachmentRefs;

        for ( size_t i = 0; i < desc.colorAttachmentDescriptors.size(); ++i )
        {
            const auto& attach = desc.colorAttachmentDescriptors[i];
            if ( attach.format == PixelFormat::INVALID )
            {
                break;
            }

            colorAttachments.push_back( {} );
            colorAttachments[i].format         = PGToVulanPixelFormat( attach.format );
            colorAttachments[i].samples        = VK_SAMPLE_COUNT_1_BIT;
            colorAttachments[i].loadOp         = PGToVulkanLoadAction( attach.loadAction );
            colorAttachments[i].storeOp        = PGToVulkanStoreAction( attach.storeAction );
            colorAttachments[i].stencilLoadOp  = PGToVulkanLoadAction( LoadAction::DONT_CARE );
            colorAttachments[i].stencilStoreOp = PGToVulkanStoreAction( StoreAction::DONT_CARE );
            colorAttachments[i].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachments[i].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            colorAttachmentRefs.push_back( {} );
            colorAttachmentRefs[i].attachment = static_cast< uint32_t>( i );
            colorAttachmentRefs[i].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = static_cast< uint32_t >( colorAttachmentRefs.size() );
        subpass.pColorAttachments    = colorAttachmentRefs.data();

        VkSubpassDependency dependency = {};
        dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass    = 0;
        dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast< uint32_t >( colorAttachments.size() );
        renderPassInfo.pAttachments    = colorAttachments.data();
        renderPassInfo.subpassCount    = 1;
        renderPassInfo.pSubpasses      = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies   = &dependency;

        if ( vkCreateRenderPass( g_renderState.device.GetNativeHandle(), &renderPassInfo, nullptr, &pass.m_handle ) != VK_SUCCESS )
        {
            pass.m_handle = VK_NULL_HANDLE;
        }

        return pass;
    }

    void RenderPass::Free()
    {
        if ( m_handle != VK_NULL_HANDLE )
        {
            vkDestroyRenderPass( g_renderState.device.GetNativeHandle(), m_handle, nullptr );
            m_handle = VK_NULL_HANDLE;
        }
    }
        
    VkRenderPass RenderPass::GetNativeHandle() const
    {
        return m_handle;
    }
    
    RenderPass::operator bool() const
    {
        return m_handle != VK_NULL_HANDLE;
    }

} // namespace Gfx
} // namespace Progression