#include "graphics/graphics_api/descriptor.hpp"
#include "core/assert.hpp"

namespace Progression
{
namespace Gfx
{

    VkDescriptorSet DescriptorSet::GetHandle() const
    {
        return m_handle;
    }

    DescriptorSet::operator bool() const
    {
        return m_handle != VK_NULL_HANDLE;
    }


    void DescriptorPool::Free()
    {
        PG_ASSERT( m_handle != VK_NULL_HANDLE );
        vkDestroyDescriptorPool( m_device, m_handle, nullptr );
        m_handle = VK_NULL_HANDLE;
    }

    std::vector< DescriptorSet > DescriptorPool::NewDescriptorSets( uint32_t numLayouts, VkDescriptorSetLayout* layouts ) const
    {
        PG_ASSERT( m_handle != VK_NULL_HANDLE );
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = m_handle;
        allocInfo.descriptorSetCount = numLayouts;
        allocInfo.pSetLayouts        = layouts;


        std::vector< DescriptorSet > descriptorSets( numLayouts );
        VkResult res = vkAllocateDescriptorSets( m_device, &allocInfo, (VkDescriptorSet*) descriptorSets.data() );
        PG_ASSERT( res == VK_SUCCESS );

        return descriptorSets;
    }
    
    VkDescriptorPool DescriptorPool::GetHandle() const
    {
        return m_handle;
    }

    DescriptorPool::operator bool() const
    {
        return m_handle != VK_NULL_HANDLE;
    }

} // namespace Gfx
} // namespace Progression
