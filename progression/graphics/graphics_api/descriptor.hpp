#pragma once

#include <vulkan/vulkan.hpp>

namespace Progression
{
namespace Gfx
{

    class DescriptorSet
    {
    public:
        DescriptorSet() = default;

        VkDescriptorSet GetNativeHandle() const;
        operator bool() const;

    private:
        VkDescriptorSet m_handle = VK_NULL_HANDLE;
    };

    class DescriptorPool 
    {
        friend class Device;
    public:
        DescriptorPool() = default;

        void Free();
        std::vector< DescriptorSet > NewDescriptorSets( uint32_t numLayouts, VkDescriptorSetLayout* layouts ) const;
        VkDescriptorPool GetNativeHandle() const;
        operator bool() const;

    private:
        VkDescriptorPool m_handle = VK_NULL_HANDLE;
        VkDevice m_device         = VK_NULL_HANDLE;
    };

} // namespace Gfx
} // namespace Progression
