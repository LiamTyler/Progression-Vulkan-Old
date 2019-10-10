#pragma once

#include "graphics/graphics_api/buffer.hpp"

namespace Progression
{
namespace Gfx
{

    enum class VertexInputRate
    {
        PER_VERTEX   = 0,
        PER_INSTANCE = 1
    };

    class VertexBindingDescriptor
    {
    public:
        uint32_t binding;
        uint32_t stride;
        VertexInputRate inputRate;
    };

    class VertexAttributeDescriptor
    {
    public:
        uint32_t location;
        uint32_t binding;
        BufferDataType format;
        uint32_t offset;
    };

    class VertexInputDescriptor
    {
    public:
        VertexInputDescriptor() = default;

        static VertexInputDescriptor Create( uint8_t numBinding, VertexBindingDescriptor* bindingDesc,
                                             uint8_t numAttrib, VertexAttributeDescriptor* attribDesc );

        const VkPipelineVertexInputStateCreateInfo& GetHandle();

    private:
        VkPipelineVertexInputStateCreateInfo m_createInfo;
        std::vector< VkVertexInputBindingDescription > m_vkBindingDescs;
        std::vector< VkVertexInputAttributeDescription > m_vkAttribDescs;
    };

} // namespace Gfx
} // namespace Progression