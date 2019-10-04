#pragma once

#include "graphics/graphics_api/render_pass.hpp"
#include "graphics/graphics_api/vertex_descriptor.hpp"
#include <vulkan/vulkan.hpp>

namespace Progression
{

class Shader;

namespace Gfx
{

    enum class CompareFunction
    {
        NEVER   = 0,
        LESS    = 1,
        LEQUAL  = 2,
        EQUAL   = 3,
        GEQUAL  = 4,
        GREATER = 5,
        NEQUAL  = 6,
        ALWAYS  = 7,

        NUM_COMPARE_FUNCTION
    };

    class PipelineDepthInfo
    {
    public:
        bool depthTestEnabled       = true;
        bool depthWriteEnabled      = true;
        CompareFunction compareFunc = CompareFunction::LESS; 
    };

        enum class BlendFactor
    {
        ZERO                    = 0,
        ONE                     = 1,
        SRC_COLOR               = 2,
        ONE_MINUS_SRC_COLOR     = 3,
        SRC_ALPHA               = 4,
        ONE_MINUS_SRC_ALPHA     = 5,
        DST_COLOR               = 6,
        ONE_MINUS_DST_COLOR     = 7,
        DST_ALPHA               = 8,
        ONE_MINUS_DST_ALPHA     = 9,
        SRC_ALPHA_SATURATE      = 10,

        NUM_BLEND_FACTORS
    };

    enum class BlendEquation
    {
        ADD                 = 0,
        SUBTRACT            = 1,
        REVERSE_SUBTRACT    = 2,
        MIN                 = 3,
        MAX                 = 4,

        NUM_BLEND_EQUATIONS
    };

    class PipelineColorAttachmentInfo
    {
    public:
        BlendFactor srcColorBlendFactor;
        BlendFactor dstColorBlendFactor;
        BlendFactor srcAlphaBlendFactor;
        BlendFactor dstAlphaBlendFactor;
        BlendEquation colorBlendEquation = BlendEquation::ADD;
        BlendEquation alphaBlendEquation = BlendEquation::ADD;
        bool blendingEnabled             = false;
    };

    enum class WindingOrder
    {
        COUNTER_CLOCKWISE = 0,
        CLOCKWISE         = 1,

        NUM_WINDING_ORDER
    };

    enum class CullFace
    {
        NONE            = 0,
        FRONT           = 1,
        BACK            = 2,
        FRONT_AND_BACK  = 3,

        NUM_CULL_FACE
    };

        enum class PolygonMode
    {
        FILL  = 0,
        LINE  = 1,
        POINT = 2,

        NUM_POLYGON_MODES
    };

    class RasterizerInfo
    {
    public:
        WindingOrder winding    = WindingOrder::COUNTER_CLOCKWISE;
        CullFace cullFace       = CullFace::BACK;
        PolygonMode polygonMode = PolygonMode::FILL;
    };

    struct Viewport
    {
        float x = 0;
        float y = 0;
        float width;
        float height;
        float minDepth = 0.0f;
        float maxDepth = 1.0f;
    };

    struct Scissor
    {
        int x = 0;
        int y = 0;
        int width;
        int height;
    };

    enum class PrimitiveType
    {
        POINTS          = 0,

        LINES           = 1,
        LINE_STRIP      = 2,

        TRIANGLES       = 3,
        TRIANGLE_STRIP  = 4,
        TRIANGLE_FAN    = 5,

        NUM_PRIMITIVE_TYPE
    };

    class PipelineDescriptor
    {
    public:
        uint8_t numShaders = 0;
        std::array< Shader*, 3 > shaders;
        VertexInputDescriptor vertexDescriptor;
        Viewport viewport;
        Scissor scissor;
        RenderPass* renderPass;
        RasterizerInfo rasterizerInfo;
        PrimitiveType primitiveType = PrimitiveType::TRIANGLES;
        PipelineDepthInfo depthInfo;
        uint8_t numColorAttachments = 0;
        std::array< PipelineColorAttachmentInfo, 8 > colorAttachmentInfos;
    };

    class Pipeline
    {
    public:
        Pipeline() = default;

        static Pipeline Create( const PipelineDescriptor& desc );
        void Free();
        VkPipeline GetNativeHandle() const;
        operator bool() const;

    private:
        PipelineDescriptor m_desc;
        VkPipeline m_pipeline             = VK_NULL_HANDLE;
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    };

} // namespace Gfx
} // namespace Progression