#include "graphics/graphics_api/pipeline.hpp"
#include "core/assert.hpp"
#include "graphics/pg_to_vulkan_types.hpp"
#include "graphics/vulkan.hpp"
#include "resource/shader.hpp"

namespace Progression
{
namespace Gfx
{

    void Pipeline::Free()
    {
        PG_ASSERT( m_pipeline != VK_NULL_HANDLE && m_pipelineLayout != VK_NULL_HANDLE );
        vkDestroyPipeline( m_device, m_pipeline, nullptr );
        vkDestroyPipelineLayout( m_device, m_pipelineLayout, nullptr );
        m_pipeline       = VK_NULL_HANDLE;
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    VkPipeline Pipeline::GetHandle() const
    {
        return m_pipeline;
    }

    VkPipelineLayout Pipeline::GetLayoutHandle() const
    {
        return m_pipelineLayout;
    }

    Pipeline::operator bool() const
    {
        return m_pipeline != VK_NULL_HANDLE;
    }

} // namespace Gfx
} // namespace Progression
