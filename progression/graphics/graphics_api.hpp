#pragma once

#include "core/common.hpp"
#include "utils/noncopyable.hpp"
#include <vector>

namespace Progression
{
namespace Gfx
{

    enum class BlendFactor
    {
        ZERO                        = 0,
        ONE                         = 1,
        SRC_COLOR                   = 2,
        ONE_MINUS_SRC_COLOR         = 3,
        SRC_ALPHA                   = 4,
        ONE_MINUS_SRC_ALPHA         = 5,
        DST_COLOR                   = 6,
        ONE_MINUS_DST_COLOR         = 7,
        DST_ALPHA                   = 8,
        ONE_MINUS_DST_ALPHA         = 9,
        SRC_ALPHA_SATURATE          = 10,
    };

    enum class BlendEquation
    {
        ADD                 = 0,
        SUBTRACT            = 1,
        REVERSE_SUBTRACT    = 2,
        MIN                 = 3,
        MAX                 = 4,
    };

    void EnableBlending( bool b );
    void SetBlendEquations( BlendEquation color, BlendEquation alpha );
    void SetBlendFactors( BlendFactor srcColor, BlendFactor dstColor, BlendFactor srcAlpha, BlendFactor dstAlpha );

    enum class WindingOrder
    {
        CLOCKWISE         = 0,
        COUNTER_CLOCKWISE = 1,
    };

    enum class CullFace
    {
        NONE            = 0,
        FRONT           = 1,
        BACK            = 2,
        FRONT_AND_BACK  = 3,
    };

    void SetWindingOrder( WindingOrder order );
    void SetCullFace( CullFace cullFace );


    struct Viewport
    {
        int x;
        int y;
        int width;
        int height;
    };

    void SetViewport( const Viewport& viewport );

    struct Scissor
    {
        int x;
        int y;
        int width;
        int height;
    };

    void SetScissor( const Scissor& viewport );

    /** \brief Blit's the data specified by mask of fromFbo to toFbo.
     *         When reading from the fromFbo, it will read whichever attachment is currently
     *         bound using glReadBuffer. This defaults to GL_BACK for the default fbo, otherwise
     *         it is GL_COLOR_ATTACHMENT0. When blitting to the toFbo, it will blit to whatever
     *         is bound using glDrawBuffers.
     *
     *         Mask needs to be GL_[COLOR,DEPTH,STENCIL]_BUFFER_BIT
     *
     *         Note: Need to test this for multiple color attachments
     */
    void blitFboToFbo( GLuint fromFbo, GLuint toFbo, int width, int height, GLbitfield mask );

    enum class BufferType
    {
        VERTEX = 0,
        INDEX,
    };

    enum class BufferUsage
    {
        STATIC  = 0,
        DYNAMIC = 1,
        STREAM  = 2
    };

    enum class BufferDataType
    {
        FLOAT16 = 0,
        FLOAT32,
        BYTE,
        UNSIGNED_BYTE,
        SHORT,
        UNSIGNED_SHORT,
        INT,
        UNSIGNED_INT
    };

    constexpr int SizeOfBufferDataType( BufferDataType type )
    {
        int size[] =
        {
            2, // FLOAT16
            4, // FLOAT32
            1, // BYTE
            1, // UNSIGNED_BYTE
            2, // SHORT
            2, // UNSIGNED_SHORT
            4, // INT
            4, // UNSIGNED_INT
        };

        return size[static_cast< int >( type )];
    }

    enum class IndexType
    {
        UNSIGNED_SHORT = 0,
        UNSIGNED_INT
    };

    constexpr int SizeOfIndexType( IndexType type )
    {
        int size[] =
        {
            2, // UNSIGNED_SHORT
            4, // UNSIGNED_INT
        };

        return size[static_cast< int >( type )];
    }

    class Buffer : public NonCopyable
    {
    public:
        Buffer() = default;
        ~Buffer();
        Buffer( Buffer&& buff );
        Buffer& operator=( Buffer&& buff );

        static Buffer Create( void* data, uint32_t length, BufferType type, BufferUsage usage );
        void SetData( void* src, uint32_t length );
        void SetData( void* src, uint32_t offset, uint32_t length );
        uint32_t GetLength() const;
        BufferType GetType() const;
        BufferUsage GetUsage() const;
        GLuint GetNativeHandle() const;

        operator bool() const;

    protected:
        void Bind() const;

        BufferType m_type;
        BufferUsage m_usage;
        uint32_t m_length       = 0; // in bytes
        GLuint m_nativeHandle   = ~0u;
    };

    void BindVertexBuffer( const Buffer& buffer, uint32_t index, int offset, uint32_t stride );
    void BindIndexBuffer( const Buffer& buffer );

    class VertexAttributeDescriptor
    {
    public:
        uint8_t binding;
        uint8_t location;
        uint8_t count;
        BufferDataType format;
        uint32_t offset;
    };

    class VertexInputDescriptor : public NonCopyable
    {
    public:
        VertexInputDescriptor() = default;
        ~VertexInputDescriptor();
        VertexInputDescriptor( VertexInputDescriptor&& desc );
        VertexInputDescriptor& operator=( VertexInputDescriptor&& desc );

        void Bind() const;
        static VertexInputDescriptor Create( uint8_t count,
                                             VertexAttributeDescriptor* attribDescriptors );
        operator bool() const;

    private:
        GLuint m_vao = ~0u;
    };

    enum class PrimitiveType
    {
        POINTS = 0,

        LINES,
        LINE_STRIP,

        TRIANGLES,
        TRIANGLE_STRIP,
        TRIANGLE_FAN,
    };

    void DrawIndexedPrimitives( PrimitiveType primType, IndexType indexType, uint32_t offset, uint32_t count );
    void DrawNonIndexedPrimitives( PrimitiveType primType, uint32_t vertexStart, uint32_t vertexCount );


    // textures

    enum class FilterMode
    {
        NEAREST                = 0,
        LINEAR                 = 1,
        NEAREST_MIPMAP_NEAREST = 2,
        LINEAR_MIPMAP_NEAREST  = 3,
        NEAREST_MIPMAP_LINEAR  = 4,
        LINEAR_MIPMAP_LINEAR   = 5,
    };

    enum class WrapMode
    {
        REPEAT          = 0,
        MIRRORED_REPEAT = 1,
        CLAMP_TO_EDGE   = 2,
        CLAMP_TO_BORDER = 3,
    };

    struct SamplerDescriptor
    {
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
        operator bool() const;

    private:
        SamplerDescriptor m_desc;
        GLuint m_nativeHandle = ~0u;
    };

    enum class PixelFormat
    {
        R8_Uint   = 0,
        R16_Float = 1,
        R32_Float = 2,

        R8_G8_Uint    = 3,
        R16_G16_Float = 4,
        R32_G32_Float = 5,

        R8_G8_B8_Uint     = 6,
        R16_G16_B16_Float = 7,
        R32_G32_B32_Float = 8,

        R8_G8_B8_A8_Uint      = 9,
        R16_G16_B16_A16_Float = 10,
        R32_G32_B32_A32_Float = 11,

        R8_G8_B8_Uint_sRGB    = 12,
        R8_G8_B8_A8_Uint_sRGB = 13,

        R11_G11_B10_Float = 14,

        DEPTH32_Float = 15,
    };

    enum class TextureType
    {
        TEXTURE2D = 0,
    };

    struct TextureDescriptor
    {
    public:
        TextureType type;
        uint32_t width  = 0;
        uint32_t height = 0;
        PixelFormat format;
        bool mipmapped = true;
    };

    class Texture : public NonCopyable
    {
    public:
        Texture() = default;
        ~Texture();
        Texture( Texture&& tex );
        Texture& operator=( Texture&& tex );

        static Texture Create( const TextureDescriptor& desc, void* data, PixelFormat srcFormat );
        // void Bind( uint32_t ) const;
        TextureType GetType() const;
        uint32_t GetWidth() const;
        uint32_t GetHeight() const;
        PixelFormat GetFormat() const;
        bool GetMipMapped() const;
        GLuint GetNativeHandle() const;
        operator bool() const;

    private:
        TextureDescriptor m_desc;
        GLuint m_nativeHandle = ~0u;
    };


    enum class LoadAction
    {
        LOAD      = 0,
        CLEAR     = 1,
        DONT_CARE = 2,
    };

    // Note: In opengl the store action is always store. Just adding this for future support of modern apis
    enum class StoreAction
    {
        STORE     = 0,
        DONT_CARE = 1,
    };

    class ColorAttachmentDescriptor
    {
    public:
        ColorAttachmentDescriptor() = default;

        
        glm::vec4 clearColor = glm::vec4( 0 );
        LoadAction loadAction = LoadAction::CLEAR;
        StoreAction storeAction = StoreAction::STORE;
        Texture* texture = nullptr;
    };

    enum class CompareFunction
    {
        NEVER = 0,
        LESS = 1,
        LEQUAL = 2,
        EQUAL = 3,
        GEQUAL = 4,
        GREATER = 5,
        NEQUAL = 6,
        ALWAYS = 7,
    };

    class DepthAttachmentDescriptor
    {
    public:
        DepthAttachmentDescriptor() = default;

        bool depthTestEnabled  = true;
        bool depthWriteEnabled = true;
        CompareFunction compareFunc = CompareFunction::LESS;
        float clearValue = 1.0f;
        LoadAction loadAction = LoadAction::CLEAR;
        StoreAction storeAction = StoreAction::STORE;
        Texture* texture = nullptr;
    };

    class RenderPassDescriptor
    {
    friend class RenderPass;
    public:
        RenderPassDescriptor() = default;

        void SetColorAttachments( const std::vector< ColorAttachmentDescriptor >& attachments );
        void SetDepthAttachment( const DepthAttachmentDescriptor& attachment );
        std::vector< ColorAttachmentDescriptor > GetColorAttachmentDescriptors() const;
        DepthAttachmentDescriptor GetDepthAttachmentDescriptor() const;
    
    private:
        std::vector< ColorAttachmentDescriptor > m_colorAttachmentDescriptors;
        DepthAttachmentDescriptor m_depthAttachmentDescriptor;
        bool m_hasDepthAttachment = false;
    };

    class RenderPass : public NonCopyable
    {
    public:
        RenderPass() = default;
        ~RenderPass();
        RenderPass( RenderPass&& r );
        RenderPass& operator=( RenderPass&& r );

        void Bind() const;

        static RenderPass CreateDefault( const RenderPassDescriptor& desc );
        static RenderPass Create( const RenderPassDescriptor& desc );
        GLuint GetNativeHandle() const;

    private:
        RenderPassDescriptor m_desc;
        GLuint m_nativeHandle = 0;
        bool m_allColorAttachmentsSameColor;
    };


    // render buffers and framebuffers

    /** \brief Creates a render buffer with the given width, height, and internal format, and
     *         binds the buffer as well. */
    GLuint createRenderbuffer( int width, int height, GLenum internalFormat = GL_DEPTH_COMPONENT );

    /** \brief Deletes the rbo given. */
    void deleteRenderbuffer( GLuint rbo );

} // namespace Gfx
} // namespace Progression
