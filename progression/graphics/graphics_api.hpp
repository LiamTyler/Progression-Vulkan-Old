#pragma once

#include "core/core_defines.hpp"
#include "core/math.hpp"
#include "utils/noncopyable.hpp"
#include <array>
#include <vulkan/vulkan.hpp>

namespace Progression
{

class Image;

namespace Gfx
{

    enum class ShaderStage
    {
        VERTEX                  = 0x00000001,
        TESSELLATION_CONTROL    = 0x00000002,
        TESSELLATION_EVALUATION = 0x00000004,
        GEOMETRY                = 0x00000008,
        FRAGMENT                = 0x00000010,
        COMPUTE                 = 0x00000020,
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

    enum class WindingOrder
    {
        CLOCKWISE         = 0,
        COUNTER_CLOCKWISE = 1,

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
        LINES = 1,

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
        float x;
        float y;
        float width;
        float height;
        float minDepth;
        float maxDepth;
    };

    struct Scissor
    {
        int x;
        int y;
        int width;
        int height;
    };

    enum class BufferType
    {
        VERTEX = 0,
        INDEX  = 1,

        NUM_BUFFER_TYPE
    };

    enum class BufferUsage
    {
        STATIC  = 0,
        DYNAMIC = 1,
        STREAM  = 2,

        NUM_BUFFER_USAGE
    };

    enum class BufferDataType
    {
        INVALID = 0,

        UCHAR  = 1,
        UCHAR2 = 2,
        UCHAR3 = 3,
        UCHAR4 = 4,

        CHAR  = 5,
        CHAR2 = 6,
        CHAR3 = 7,
        CHAR4 = 8,

        UCHAR_NORM  = 9,
        UCHAR2_NORM = 10,
        UCHAR3_NORM = 11,
        UCHAR4_NORM = 12,

        CHAR_NORM  = 13,
        CHAR2_NORM = 14,
        CHAR3_NORM = 15,
        CHAR4_NORM = 16,

        USHORT  = 17,
        USHORT2 = 18,
        USHORT3 = 19,
        USHORT4 = 20,

        SHORT  = 21,
        SHORT2 = 22,
        SHORT3 = 23,
        SHORT4 = 24,

        USHORT_NORM  = 25,
        USHORT2_NORM = 26,
        USHORT3_NORM = 27,
        USHORT4_NORM = 28,

        SHORT_NORM  = 29,
        SHORT2_NORM = 30,
        SHORT3_NORM = 31,
        SHORT4_NORM = 32,

        HALF  = 33,
        HALF2 = 34,
        HALF3 = 35,
        HALF4 = 36,

        FLOAT  = 37,
        FLOAT2 = 38,
        FLOAT3 = 39,
        FLOAT4 = 40,

        UINT  = 41,
        UINT2 = 42,
        UINT3 = 43,
        UINT4 = 44,

        INT  = 45,
        INT2 = 46,
        INT3 = 47,
        INT4 = 48,

        NUM_BUFFER_DATA_TYPE
    };

    enum class IndexType
    {
        UNSIGNED_SHORT = 0,
        UNSIGNED_INT,

        NUM_INDEX_TYPE
    };

    constexpr int SizeOfIndexType( IndexType type )
    {
        int size[] =
        {
            2, // UNSIGNED_SHORT
            4, // UNSIGNED_INT
        };

        static_assert( ARRAY_COUNT( size ) == static_cast< int >( IndexType::NUM_INDEX_TYPE ) );

        return size[static_cast< int >( type )];
    }

    class Buffer : public NonCopyable
    {
    public:
        Buffer() = default;
        ~Buffer();
        Buffer( Buffer&& buff );
        Buffer& operator=( Buffer&& buff );

        static Buffer Create( void* data, size_t length, BufferType type, BufferUsage usage );
        void SetData( void* src, size_t length );
        void SetData( void* src, size_t offset, size_t length );
        size_t GetLength() const;
        BufferType GetType() const;
        BufferUsage GetUsage() const;
        // GLuint GetNativeHandle() const;
        // operator bool() const;

    protected:
        void Bind() const;

        BufferType m_type;
        BufferUsage m_usage;
        size_t m_length       = 0; // in bytes
        // GLuint m_nativeHandle = ~0u;
    };

    //void BindVertexBuffer( const Buffer& buffer, uint32_t index, int offset, uint32_t stride );
    //void BindIndexBuffer( const Buffer& buffer );

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

    class VertexInputDescriptor : public NonCopyable
    {
    public:
        VertexInputDescriptor() = default;
        // ~VertexInputDescriptor();
        //VertexInputDescriptor( VertexInputDescriptor&& desc );
        //VertexInputDescriptor& operator=( VertexInputDescriptor&& desc );

        static VertexInputDescriptor Create( uint8_t numBinding, VertexBindingDescriptor* bindingDesc,
                                             uint8_t numAttrib, VertexAttributeDescriptor* attribDesc );

    private:
        VkPipelineVertexInputStateCreateInfo m_createInfo;
        std::vector< VkVertexInputBindingDescription > m_vkBindingDescs;
        std::vector< VkVertexInputAttributeDescription > m_vkAttribDescs;
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

    //void DrawIndexedPrimitives( PrimitiveType primType, IndexType indexType, uint32_t offset, uint32_t count );
    //void DrawNonIndexedPrimitives( PrimitiveType primType, uint32_t vertexStart, uint32_t vertexCount );

    enum class FilterMode
    {
        NEAREST                = 0,
        LINEAR                 = 1,
        NEAREST_MIPMAP_NEAREST = 2,
        LINEAR_MIPMAP_NEAREST  = 3,
        NEAREST_MIPMAP_LINEAR  = 4,
        LINEAR_MIPMAP_LINEAR   = 5,

        NUM_FILTER_MODE
    };

    enum class WrapMode
    {
        REPEAT          = 0,
        MIRRORED_REPEAT = 1,
        CLAMP_TO_EDGE   = 2,
        CLAMP_TO_BORDER = 3,

        NUM_WRAP_MODE
    };

    class SamplerDescriptor
    {
    public:
        FilterMode minFilter  = FilterMode::LINEAR;
        FilterMode magFilter  = FilterMode::LINEAR;
        WrapMode wrapModeS    = WrapMode::CLAMP_TO_EDGE;
        WrapMode wrapModeT    = WrapMode::CLAMP_TO_EDGE;
        WrapMode wrapModeR    = WrapMode::CLAMP_TO_EDGE;
        float maxAnisotropy   = 1.0f;
        glm::vec4 borderColor = glm::vec4( 0 );
    };

    class Sampler : public NonCopyable
    {
    public:
        Sampler() = default;
        ~Sampler();
        Sampler( Sampler&& s );
        Sampler& operator=( Sampler&& s );

        static Sampler Create( const SamplerDescriptor& desc );
        void Bind( uint32_t index ) const;

        FilterMode GetMinFilter() const;
        FilterMode GetMagFilter() const;
        WrapMode GetWrapModeS() const;
        WrapMode GetWrapModeT() const;
        WrapMode GetWrapModeR() const;
        float GetMaxAnisotropy() const;
        glm::vec4 GetBorderColor() const;
        // operator bool() const;

    private:
        SamplerDescriptor m_desc;
        // GLuint m_nativeHandle = ~0u;
    };

    enum class PixelFormat : uint8_t
    {
        R8_UINT                 = 0,
        R16_FLOAT               = 1,
        R32_FLOAT               = 2,

        R8_G8_UINT              = 3,
        R16_G16_FLOAT           = 4,
        R32_G32_FLOAT           = 5,

        R8_G8_B8_UINT           = 6,
        R16_G16_B16_FLOAT       = 7,
        R32_G32_B32_FLOAT       = 8,

        R8_G8_B8_A8_UINT        = 9,
        R16_G16_B16_A16_FLOAT   = 10,
        R32_G32_B32_A32_FLOAT   = 11,

        R8_G8_B8_UINT_SRGB      = 12,
        R8_G8_B8_A8_UINT_SRGB   = 13,

        R11_G11_B10_FLOAT       = 14,

        DEPTH32_FLOAT           = 15,

        BC1_RGB_UNORM           = 16,
        BC1_RGB_SRGB            = 17,
        BC1_RGBA_UNORM          = 18,
        BC1_RGBA_SRGB           = 19,
        BC2_UNORM               = 20,
        BC2_SRGB                = 21,
        BC3_UNORM               = 22,
        BC3_SRGB                = 23,
        BC4_UNORM               = 24,
        BC4_SNORM               = 25,
        BC5_UNORM               = 26,
        BC5_SNORM               = 27,
        BC6H_UFLOAT             = 28,
        BC6H_SFLOAT             = 29,
        BC7_UNORM               = 30,
        BC7_SRGB                = 31,

        NUM_PIXEL_FORMATS
    };

    int SizeOfPixelFromat( const PixelFormat& format );
    bool PixelFormatIsCompressed( const PixelFormat& format );

    enum class ImageType : uint8_t
    {
        TYPE_1D            = 0,
        TYPE_1D_ARRAY      = 1,
        TYPE_2D            = 2,
        TYPE_2D_ARRAY      = 3,
        TYPE_CUBEMAP       = 4,
        TYPE_CUBEMAP_ARRAY = 5,
        TYPE_3D            = 6,

        NUM_IMAGE_TYPES
    };

    class ImageDescriptor
    {
    public:
        ImageType type        = ImageType::NUM_IMAGE_TYPES;
        PixelFormat srcFormat = PixelFormat::NUM_PIXEL_FORMATS;
        PixelFormat dstFormat = PixelFormat::NUM_PIXEL_FORMATS;
        uint8_t mipLevels     = 1;
        uint8_t arrayLayers   = 1;
        uint32_t width        = 0;
        uint32_t height       = 0;
        uint32_t depth        = 1;
    };

    class Texture : public NonCopyable
    {
        friend class ::Progression::Image;
    public:
        Texture() = default;
        ~Texture();
        Texture( Texture&& tex );
        Texture& operator=( Texture&& tex );

        static Texture Create( const ImageDescriptor& desc, void* data );
        void Free();
        unsigned char* GetPixelData() const;
        ImageType GetType() const;
        PixelFormat GetPixelFormat() const;
        uint8_t GetMipLevels() const;
        uint8_t GetArrayLayers() const;
        uint32_t GetWidth() const;
        uint32_t GetHeight() const;
        uint32_t GetDepth() const;
        // GLuint GetNativeHandle() const;
        // operator bool() const;

    private:
        ImageDescriptor m_desc;
        // GLuint m_nativeHandle = ~0u;
    };


    enum class LoadAction
    {
        LOAD      = 0,
        CLEAR     = 1,
        DONT_CARE = 2,

        NUM_LOAD_ACTION
    };

    // Note: In opengl the store action is always store. Just adding this for future support of modern apis
    enum class StoreAction
    {
        STORE     = 0,
        DONT_CARE = 1,

        NUM_STORE_ACTION
    };

    class ColorAttachmentDescriptor
    {
    public:
        ColorAttachmentDescriptor() = default;

        glm::vec4 clearColor    = glm::vec4( 0 );
        LoadAction loadAction   = LoadAction::CLEAR;
        StoreAction storeAction = StoreAction::STORE;
        Texture* texture        = nullptr;
    };

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

    class DepthAttachmentDescriptor
    {
    public:
        DepthAttachmentDescriptor() = default;

        float clearValue        = 1.0f;
        LoadAction loadAction   = LoadAction::CLEAR;
        StoreAction storeAction = StoreAction::STORE;
        Texture* texture        = nullptr;
    };

    class RenderPassDescriptor
    {
    friend class RenderPass;
    public:
        RenderPassDescriptor() = default;

        std::array< ColorAttachmentDescriptor, 8 > colorAttachmentDescriptors;
        DepthAttachmentDescriptor depthAttachmentDescriptor;
    };

    class RenderPass : public NonCopyable
    {
    public:
        RenderPass() = default;
        ~RenderPass();
        RenderPass( RenderPass&& r );
        RenderPass& operator=( RenderPass&& r );

        void Bind() const;

        static RenderPass Create( const RenderPassDescriptor& desc );
        // GLuint GetNativeHandle() const;

    private:
        RenderPassDescriptor m_desc;
        // GLuint m_nativeHandle = ~0u;
    };

    enum RenderTargetBuffers
    {
        RENDER_TARGET_COLOR   = 0x1,
        RENDER_TARGET_DEPTH   = 0x2,
        RENDER_TARGET_STENCIL = 0x4,

        NUM_RENDER_TARGET_BUFFERS
    };

    class PipelineDepthInfo
    {
    public:
        bool depthTestEnabled       = true;
        bool depthWriteEnabled      = true;
        CompareFunction compareFunc = CompareFunction::LESS; 
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

    class PipelineDescriptor
    {
    public:
        VertexInputDescriptor* vertexDescriptors;
        Viewport viewport               = { -1 };
        Scissor scissor                 = { -1 };
        std::array< PipelineColorAttachmentInfo, 8 > colorAttachmentInfos;
        uint8_t numColorAttachments     = 0;
        PipelineDepthInfo depthInfo     = {};
        RasterizerInfo rasterizerInfo   = {};
        PrimitiveType primitiveType     = PrimitiveType::TRIANGLES;
    };

    class Pipeline : public NonCopyable
    {
    public:
        Pipeline() = default;
        Pipeline( Pipeline&& pipeline ) = default;
        Pipeline& operator=( Pipeline&& pipeline ) = default;

        static Pipeline Create( const PipelineDescriptor& desc );

        void Bind() const;

    private:
        PipelineDescriptor m_desc;
        VertexInputDescriptor m_vertexDesc;
    };

    //void Blit( const RenderPass& src, const RenderPass& dst, int width, int height,
     //          const RenderTargetBuffers& mask, FilterMode filter );

    class Device : public NonCopyable
    {
    public:
        Device() = default;
        ~Device();
        Device( Device&& device );
        Device& operator=( Device&& device );

        void Free();

        static Device CreateDefault();

        VkDevice GetNativeHandle() const;
        operator bool() const;

    private:
        VkDevice m_handle        = VK_NULL_HANDLE;
        VkQueue  m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue  m_presentQueue  = VK_NULL_HANDLE;
    };

    extern Device g_device;

} // namespace Gfx
} // namespace Progression
