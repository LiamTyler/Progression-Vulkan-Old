#pragma once

#include "core/core_defines.hpp"
#include "core/math.hpp"
#include "utils/noncopyable.hpp"
#include <array>
#include <vulkan/vulkan.hpp>

namespace Progression
{

class Image;
class Shader;

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
        float x = -1;
        float y = -1;
        float width;
        float height;
        float minDepth = 0.0f;
        float maxDepth = 1.0f;
    };

    struct Scissor
    {
        int x = -1;
        int y = -1;
        int width;
        int height;
    };

    enum class BufferType
    {
        VERTEX = 0,
        INDEX  = 1,

        NUM_BUFFER_TYPE
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
    friend class Device;
    public:
        Buffer() = default;
        ~Buffer();
        Buffer( Buffer&& buff );
        Buffer& operator=( Buffer&& buff );

        void Free();
        void* Map();
        void UnMap();
        void Bind( size_t offset = 0 ) const;
        size_t GetLength() const;
        BufferType GetType() const;
        VkBuffer GetNativeHandle() const;
        operator bool() const;

    protected:

        BufferType m_type;
        size_t m_length; // in bytes
        VkBuffer m_handle = VK_NULL_HANDLE;
        VkDeviceMemory m_memory;
        VkDevice m_device;
    };

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

        const VkPipelineVertexInputStateCreateInfo& GetNativeHandle();

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
        void Free();

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

    enum class PixelFormat
    {
        INVALID                 = 0,

        R8_UNORM                = 1,
        R8_SNORM                = 2,
        R8_UINT                 = 3,
        R8_SINT                 = 4,
        R8_SRGB                 = 5,

        R8_G8_UNORM             = 6,
        R8_G8_SNORM             = 7,
        R8_G8_UINT              = 8,
        R8_G8_SINT              = 9,
        R8_G8_SRGB              = 10,

        R8_G8_B8_UNORM          = 11,
        R8_G8_B8_SNORM          = 12,
        R8_G8_B8_UINT           = 13,
        R8_G8_B8_SINT           = 14,
        R8_G8_B8_SRGB           = 15,

        B8_G8_R8_UNORM          = 16,
        B8_G8_R8_SNORM          = 17,
        B8_G8_R8_UINT           = 18,
        B8_G8_R8_SINT           = 19,
        B8_G8_R8_SRGB           = 20,

        R8_G8_B8_A8_UNORM       = 21,
        R8_G8_B8_A8_SNORM       = 22,
        R8_G8_B8_A8_UINT        = 23,
        R8_G8_B8_A8_SINT        = 24,
        R8_G8_B8_A8_SRGB        = 25,

        B8_G8_R8_A8_UNORM       = 26,
        B8_G8_R8_A8_SNORM       = 27,
        B8_G8_R8_A8_UINT        = 28,
        B8_G8_R8_A8_SINT        = 29,
        B8_G8_R8_A8_SRGB        = 30,

        R16_UNORM               = 31,
        R16_SNORM               = 32,
        R16_UINT                = 33,
        R16_SINT                = 34,
        R16_FLOAT               = 35,

        R16_G16_UNORM           = 36,
        R16_G16_SNORM           = 37,
        R16_G16_UINT            = 38,
        R16_G16_SINT            = 39,
        R16_G16_FLOAT           = 40,

        R16_G16_B16_UNORM       = 41,
        R16_G16_B16_SNORM       = 42,
        R16_G16_B16_UINT        = 43,
        R16_G16_B16_SINT        = 44,
        R16_G16_B16_FLOAT       = 45,

        R16_G16_B16_A16_UNORM   = 46,
        R16_G16_B16_A16_SNORM   = 47,
        R16_G16_B16_A16_UINT    = 48,
        R16_G16_B16_A16_SINT    = 49,
        R16_G16_B16_A16_FLOAT   = 50,

        R32_UINT                = 51,
        R32_SINT                = 52,
        R32_FLOAT               = 53,

        R32_G32_UINT            = 54,
        R32_G32_SINT            = 55,
        R32_G32_FLOAT           = 56,

        R32_G32_B32_UINT        = 57,
        R32_G32_B32_SINT        = 58,
        R32_G32_B32_FLOAT       = 59,

        R32_G32_B32_A32_UINT    = 60,
        R32_G32_B32_A32_SINT    = 61,
        R32_G32_B32_A32_FLOAT   = 62,

        DEPTH_16_UNORM                  = 63,
        DEPTH_32_FLOAT                  = 64,
        DEPTH_16_UNORM_STENCIL_8_UINT   = 65,
        DEPTH_24_UNORM_STENCIL_8_UINT   = 66,
        DEPTH_32_FLOAT_STENCIL_8_UINT   = 67,

        STENCIL_8_UINT          = 68,

        BC1_RGB_UNORM           = 69,
        BC1_RGB_SRGB            = 70,
        BC1_RGBA_UNORM          = 71,
        BC1_RGBA_SRGB           = 72,
        BC2_UNORM               = 73,
        BC2_SRGB                = 74,
        BC3_UNORM               = 75,
        BC3_SRGB                = 76,
        BC4_UNORM               = 77,
        BC4_SNORM               = 78,
        BC5_UNORM               = 79,
        BC5_SNORM               = 80,
        BC6H_UFLOAT             = 81,
        BC6H_SFLOAT             = 82,
        BC7_UNORM               = 83,
        BC7_SRGB                = 84,

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

        VkImage GetNativeHandle() const;
        VkImageView GetView() const;

        operator bool() const;

    private:
        ImageDescriptor m_desc;
        VkImage m_image         = VK_NULL_HANDLE;
        VkImageView m_imageView = VK_NULL_HANDLE;
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
        PixelFormat format      = PixelFormat::INVALID;
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

        static RenderPass Create( const RenderPassDescriptor& desc );
        void Free();
        VkRenderPass GetNativeHandle() const;
        operator bool() const;

        RenderPassDescriptor desc;

    private:
        VkRenderPass m_handle = VK_NULL_HANDLE;
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

    class Pipeline : public NonCopyable
    {
    public:
        Pipeline() = default;
        ~Pipeline();
        Pipeline( Pipeline&& pipeline );
        Pipeline& operator=( Pipeline&& pipeline );

        static Pipeline Create( const PipelineDescriptor& desc );
        void Free();
        VkPipeline GetNativeHandle() const;
        operator bool() const;

    private:
        PipelineDescriptor m_desc;
        VkPipeline m_pipeline             = VK_NULL_HANDLE;
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    };

    class Fence
    {
        friend class Device;
    public:
        Fence() = default;

        void Free();
        void WaitFor();
        void Reset();

    private:
        VkFence m_handle  = VK_NULL_HANDLE;
        VkDevice m_device;
    };

    class CommandBuffer : public NonCopyable
    {
        friend class CommandPool;
    public:
        CommandBuffer() = default;
        CommandBuffer( CommandBuffer&& cmdbuf );
        CommandBuffer& operator=( CommandBuffer&& cmdbuf );
        operator bool() const;
        VkCommandBuffer GetNativeHandle() const;

        bool BeginRecording();
        bool EndRecording();
        void BeginRenderPass( const RenderPass& renderPass, VkFramebuffer framebuffer );
        void EndRenderPass();
        void BindRenderPipeline( const Pipeline& pipeline );

        void Draw( uint32_t firstVert, uint32_t vertCount, uint32_t instanceCount = 1, uint32_t firstInstance = 0 );
    private:
        VkCommandBuffer m_handle = VK_NULL_HANDLE;
        VkCommandBufferBeginInfo m_beginInfo;
    };


    class CommandPool : public NonCopyable
    {
        friend class Device;
    public:
        CommandPool() = default;
        ~CommandPool();
        CommandPool( CommandPool&& pool );
        CommandPool& operator=( CommandPool&& pool );
        void Free();
        operator bool() const;

        CommandBuffer NewCommandBuffer();

    private:
        VkCommandPool m_handle = VK_NULL_HANDLE;
        VkDevice m_device      = VK_NULL_HANDLE;
    };


    class Device : public NonCopyable
    {
    public:
        Device() = default;
        ~Device();
        Device( Device&& device );
        Device& operator=( Device&& device );
        void Free();
        operator bool() const;

        static Device CreateDefault();
        CommandPool NewCommandPool() const;
        Fence NewFence() const;
        Buffer NewBuffer( size_t length, BufferType type ) const;
        void SubmitRenderCommands( int numBuffers, CommandBuffer* cmdBufs ) const;
        void SubmitFrame( uint32_t imageIndex ) const;

        VkDevice GetNativeHandle() const;
        VkQueue GraphicsQueue() const;
        VkQueue PresentQueue() const;

    private:
        VkDevice m_handle        = VK_NULL_HANDLE;
        VkQueue  m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue  m_presentQueue  = VK_NULL_HANDLE;
    };



} // namespace Gfx
} // namespace Progression
