#include "graphics/graphics_api/render_pass.hpp"
#include "graphics/pg_to_vulkan_types.hpp"

namespace Progression
{
namespace Gfx
{

    void RenderPass::Free()
    {
        PG_ASSERT( m_handle != VK_NULL_HANDLE );
        vkDestroyRenderPass( m_device, m_handle, nullptr );
        m_handle = VK_NULL_HANDLE;
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
