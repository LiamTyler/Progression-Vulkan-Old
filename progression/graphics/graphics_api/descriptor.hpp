#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace Progression
{
namespace Gfx
{

    class DescriptorSet
    {
    public:
        DescriptorSet() = default;

        VkDescriptorSet GetHandle() const;
        operator bool() const;

    private:
        VkDescriptorSet m_handle = VK_NULL_HANDLE;
    };


    struct DescriptorSetLayoutData
    {
        uint32_t setNumber;
        VkDescriptorSetLayoutCreateInfo createInfo;
        std::vector< VkDescriptorSetLayoutBinding > bindings;
    };

    std::vector< DescriptorSetLayoutData > CombineDescriptorSetLayouts( std::vector< DescriptorSetLayoutData >& layoutDatas );

    class DescriptorSetLayout
    {
        friend class Device;
    public:
        DescriptorSetLayout() = default;

        void Free();
        VkDescriptorSetLayout GetHandle() const;
        operator bool() const;

    private:
        VkDescriptorSetLayout m_handle = VK_NULL_HANDLE;
        VkDevice m_device              = VK_NULL_HANDLE;
    };


    class DescriptorPool 
    {
        friend class Device;
    public:
        DescriptorPool() = default;

        void Free();
        std::vector< DescriptorSet > NewDescriptorSets( uint32_t numLayouts, const DescriptorSetLayout& layout, const std::string& name = "" ) const;
        DescriptorSet NewDescriptorSet( const DescriptorSetLayout& layout, const std::string& name = "" ) const;
        VkDescriptorPool GetHandle() const;
        operator bool() const;

    private:
        VkDescriptorPool m_handle = VK_NULL_HANDLE;
        VkDevice m_device         = VK_NULL_HANDLE;
    };

} // namespace Gfx
} // namespace Progression
