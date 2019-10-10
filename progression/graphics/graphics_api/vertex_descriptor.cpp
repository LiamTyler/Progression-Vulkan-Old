#include "graphics/graphics_api/vertex_descriptor.hpp"
#include "graphics/pg_to_vulkan_types.hpp"

namespace Progression
{
namespace Gfx
{

    VertexInputDescriptor VertexInputDescriptor::Create( uint8_t numBinding, VertexBindingDescriptor* bindingDesc,
                                                         uint8_t numAttrib, VertexAttributeDescriptor* attribDesc )
    {
        VertexInputDescriptor desc;
        desc.m_vkBindingDescs.resize( numBinding );
        desc.m_vkAttribDescs.resize( numAttrib );
        for ( uint8_t i = 0; i < numBinding; ++i )
        {
            desc.m_vkBindingDescs[i].binding   = bindingDesc[i].binding;
            desc.m_vkBindingDescs[i].stride    = bindingDesc[i].stride;
            desc.m_vkBindingDescs[i].inputRate = PGToVulkanVertexInputRate( bindingDesc[i].inputRate );
        }

        for ( uint8_t i = 0; i < numAttrib; ++i )
        {
            desc.m_vkAttribDescs[i].location = attribDesc[i].location;
            desc.m_vkAttribDescs[i].binding  = attribDesc[i].binding;
            desc.m_vkAttribDescs[i].format   = PGToVulkanBufferDataType( attribDesc[i].format );
            desc.m_vkAttribDescs[i].offset   = attribDesc[i].offset;
        }

        desc.m_createInfo = {};
        desc.m_createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        desc.m_createInfo.vertexBindingDescriptionCount   = numBinding;
        desc.m_createInfo.pVertexBindingDescriptions      = numBinding ? desc.m_vkBindingDescs.data() : nullptr;
        desc.m_createInfo.vertexAttributeDescriptionCount = numAttrib;
        desc.m_createInfo.pVertexAttributeDescriptions    = numAttrib ? desc.m_vkAttribDescs.data() : nullptr;

        return desc;
    }

    const VkPipelineVertexInputStateCreateInfo& VertexInputDescriptor::GetHandle()
    {
        return m_createInfo;
    }

} // namespace Gfx
} // namespace Progression