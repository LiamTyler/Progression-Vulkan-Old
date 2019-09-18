#include "core/assert.hpp"
#include "graphics/graphics_api.hpp"
#include "graphics/pg_to_opengl_types.hpp"
#include "utils/logger.hpp"
#include <vector>

namespace Progression
{
namespace Gfx
{

    Buffer::~Buffer()
    {
        /*if ( m_nativeHandle != ~0u )
        {
            glDeleteBuffers( 1, &m_nativeHandle );
        }*/
    }

    Buffer::Buffer( Buffer&& buff )
    {
        *this = std::move( buff );
    }
    Buffer& Buffer::operator=( Buffer&& buff )
    {
        m_length            = std::move( buff.m_length );
        m_type              = std::move( buff.m_type );
        m_usage             = std::move( buff.m_usage );
        //m_nativeHandle      = std::move( buff.m_nativeHandle );
        //buff.m_nativeHandle = ~0u;

        return *this;
    }

    Buffer Buffer::Create( void* data, size_t length, BufferType type, BufferUsage usage )
    {
        Buffer buffer;
        //glGenBuffers( 1, &buffer.m_nativeHandle );
        buffer.m_type  = type;
        buffer.m_usage = usage;
        buffer.SetData( data, length );

        return buffer;
    }

    void Buffer::SetData( void* src, size_t length )
    {
        PG_UNUSED( src );
        PG_UNUSED( length );
        PG_ASSERT( false );
        /*
        if ( !length )
        {
            return;
        }
        Bind();
        m_length = length;
        glBufferData( PGToOpenGLBufferType( m_type ), m_length, src, PGToOpenGLBufferUsage( m_usage ) );
        */
    }
    void Buffer::SetData( void* src, size_t offset, size_t length )
    {
        PG_UNUSED( src );
        PG_UNUSED( offset );
        PG_UNUSED( length );
        PG_ASSERT( false );
        /*
        if ( !length )
        {
            return;
        }
        Bind();
        PG_ASSERT( offset + length <= m_length );
        glBufferSubData( PGToOpenGLBufferType( m_type ), offset, length, src );
        */
    }

    size_t Buffer::GetLength() const
    {
        return m_length;
    }

    BufferType Buffer::GetType() const
    {
        return m_type;
    }

    BufferUsage Buffer::GetUsage() const
    {
        return m_usage;
    }

    /*GLuint Buffer::GetNativeHandle() const
    {
        return m_nativeHandle;
    }

    Buffer::operator bool() const
    {
        return m_nativeHandle != ~0u;
    }*/

    void Buffer::Bind() const
    {
        //PG_ASSERT( m_nativeHandle != (GLuint) -1 );
        //glBindBuffer( PGToOpenGLBufferType( m_type ), m_nativeHandle );
    }

    /*void BindVertexBuffer( const Buffer& buffer, uint32_t index, int offset, uint32_t stride )
    {
        PG_ASSERT( buffer.GetNativeHandle() != ~0u );
        glBindVertexBuffer( index, buffer.GetNativeHandle(), offset, stride );
    }

    void BindIndexBuffer( const Buffer& buffer )
    {
        PG_ASSERT( buffer.GetNativeHandle() != ~0u );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, buffer.GetNativeHandle() );
    }*/

    VertexInputDescriptor::~VertexInputDescriptor()
    {
        /*if ( m_vao != static_cast< GLuint >( -1 ) )
        {
            glDeleteVertexArrays( 1, &m_vao );
        }*/
    }

    VertexInputDescriptor::VertexInputDescriptor( VertexInputDescriptor&& desc )
    {
        *this = std::move( desc );
    }

    VertexInputDescriptor& VertexInputDescriptor::operator=( VertexInputDescriptor&& desc )
    {
        //m_vao      = std::move( desc.m_vao );
        //desc.m_vao = static_cast< GLuint >( -1 );

        return *this;
    }

    void VertexInputDescriptor::Bind() const
    {
        PG_ASSERT( false );
        //glBindVertexArray( m_vao );
    }

    VertexInputDescriptor VertexInputDescriptor::Create( uint8_t count, VertexAttributeDescriptor* attribDescriptors )
    {
        VertexInputDescriptor desc;
        PG_UNUSED( count );
        PG_UNUSED( attribDescriptors );
        PG_ASSERT( false );
        /*
        glGenVertexArrays( 1, &desc.m_vao );
        glBindVertexArray( desc.m_vao );

        for ( uint8_t i = 0; i < count; ++i )
        {
            glVertexAttribFormat( attribDescriptors[i].location, attribDescriptors[i].count,
                                  PGToOpenGLBufferDataType( attribDescriptors[i].format ), false,
                                  attribDescriptors[i].offset );
            glVertexAttribBinding( attribDescriptors[i].location, attribDescriptors[i].binding );
            // which buffer binding point it is attached to
            glEnableVertexAttribArray( i );
        }
        */

        return desc;
    }

    /*VertexInputDescriptor::operator bool() const
    {
        return m_vao != ~0u;
    }*/

    /*void DrawIndexedPrimitives( PrimitiveType primType, IndexType indexType, uint32_t offset, uint32_t count )
    {
        auto glTopology  = PGToOpenGLPrimitiveType( primType );
        auto glIndexType = PGToOpenGLIndexType( indexType );
        glDrawElements( glTopology, count, glIndexType, (void*) (uint64_t) ( offset * SizeOfIndexType( indexType ) ) );
    }

    void DrawNonIndexedPrimitives( PrimitiveType primType, uint32_t vertexStart, uint32_t vertexCount )
    {
        auto glTopology  = PGToOpenGLPrimitiveType( primType );
        glDrawArrays( glTopology, vertexStart, vertexCount );
    }*/

    Sampler::~Sampler()
    {
        /*if ( m_nativeHandle != ~0u )
        {
            glDeleteSamplers( 1, &m_nativeHandle );
        }*/
    }

    Sampler::Sampler( Sampler&& s )
    {
        *this = std::move( s );
    }

    Sampler& Sampler::operator=( Sampler&& s )
    {
        m_desc           = std::move( s.m_desc );
        //m_nativeHandle   = std::move( s.m_nativeHandle );
        //s.m_nativeHandle = ~0u;

        return *this;
    }

    Sampler Sampler::Create( const SamplerDescriptor& desc )
    {
        Sampler sampler;

        PG_UNUSED( desc );
        PG_ASSERT( false );
        /*
        sampler.m_desc = desc;
        glGenSamplers( 1, &sampler.m_nativeHandle );

        auto nativeWrapS     = PGToOpenGLWrapMode( desc.wrapModeS );
        auto nativeWrapT     = PGToOpenGLWrapMode( desc.wrapModeS );
        auto nativeWrapR     = PGToOpenGLWrapMode( desc.wrapModeS );
        auto nativeMinFilter = PGToOpenGLFilterMode( desc.minFilter );
        auto nativeMagFilter = PGToOpenGLFilterMode( desc.magFilter );

        glSamplerParameteri( sampler.m_nativeHandle, GL_TEXTURE_WRAP_S, nativeWrapS );
        glSamplerParameteri( sampler.m_nativeHandle, GL_TEXTURE_WRAP_T, nativeWrapT );
        glSamplerParameteri( sampler.m_nativeHandle, GL_TEXTURE_WRAP_R, nativeWrapR );
        glSamplerParameteri( sampler.m_nativeHandle, GL_TEXTURE_MIN_FILTER, nativeMinFilter );
        glSamplerParameteri( sampler.m_nativeHandle, GL_TEXTURE_MAG_FILTER, nativeMagFilter );
        glSamplerParameterfv( sampler.m_nativeHandle, GL_TEXTURE_BORDER_COLOR,
                              glm::value_ptr( desc.borderColor ) );
        glSamplerParameterf( sampler.m_nativeHandle, GL_TEXTURE_MAX_ANISOTROPY, desc.maxAnisotropy );
        */

        return sampler;
    }

    void Sampler::Bind( uint32_t index ) const
    {
        PG_UNUSED( index );
        PG_ASSERT( false );
        //PG_ASSERT( m_nativeHandle != ~0u );
        //glBindSampler( index, m_nativeHandle );
    }


    FilterMode Sampler::GetMinFilter() const
    {
        return m_desc.minFilter;
    }

    FilterMode Sampler::GetMagFilter() const
    {
        return m_desc.magFilter;
    }

    WrapMode Sampler::GetWrapModeS() const
    {
        return m_desc.wrapModeS;
    }

    WrapMode Sampler::GetWrapModeT() const
    {
        return m_desc.wrapModeT;
    }

    WrapMode Sampler::GetWrapModeR() const
    {
        return m_desc.wrapModeR;
    }

    float Sampler::GetMaxAnisotropy() const
    {
        return m_desc.maxAnisotropy;
    }

    glm::vec4 Sampler::GetBorderColor() const
    {
        return m_desc.borderColor;
    }

    /*Sampler::operator bool() const
    {
        return m_nativeHandle != ~0u;
    }*/

    int SizeOfPixelFromat( const PixelFormat& format )
    {
        int size[] =
        {
            1,  // R8_UINT
            2,  // R16_FLOAT
            4,  // R32_FLOAT

            2,  // R8_G8_UINT
            4,  // R16_G16_FLOAT
            8,  // R32_G32_FLOAT

            3,  // R8_G8_B8_UINT
            6,  // R16_G16_B16_FLOAT
            12, // R32_G32_B32_FLOAT

            4,  // R8_G8_B8_A8_UINT
            8,  // R16_G16_B16_A16_FLOAT
            16, // R32_G32_B32_A32_FLOAT

            3,  // R8_G8_B8_UINT_SRGB
            4,  // R8_G8_B8_A8_UINT_SRGB

            4,  // R11_G11_B10_FLOAT

            4,  // DEPTH32_FLOAT

            // Pixel size for the BC formats is the size of the 4x4 block, not per pixel
            8,  // BC1_RGB_UNORM
            8,  // BC1_RGB_SRGB
            8,  // BC1_RGBA_UNORM
            8,  // BC1_RGBA_SRGB
            16, // BC2_UNORM
            16, // BC2_SRGB
            16, // BC3_UNORM
            16, // BC3_SRGB
            8,  // BC4_UNORM
            8,  // BC4_SNORM
            16, // BC5_UNORM
            16, // BC5_SNORM
            16, // BC6H_UFLOAT
            16, // BC6H_SFLOAT
            16, // BC7_UNORM
            16, // BC7_SRGB
        };

        PG_ASSERT( static_cast< int >( format ) < static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS ) );
        static_assert( ARRAY_COUNT( size ) == static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS ) );

        return size[static_cast< int >( format )];
    }

    bool PixelFormatIsCompressed( const PixelFormat& format )
    {
        int f = static_cast< int >( format );
        return static_cast< int >( PixelFormat::DEPTH32_FLOAT ) < f && f < static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS );
    }

    Texture::~Texture()
    {
        Free();
    }

    Texture::Texture( Texture&& tex )
    {
        *this = std::move( tex );
    }
    Texture& Texture::operator=( Texture&& tex )
    {
        m_desc              = std::move( tex.m_desc );
        //m_nativeHandle      = std::move( tex.m_nativeHandle );
        //tex.m_nativeHandle  = ~0u;

        return *this;
    }

    Texture Texture::Create( const ImageDescriptor& desc, void* data )
    {
        PG_UNUSED( desc );
        PG_UNUSED( data );
        PG_ASSERT( false );
        Texture tex;
        return tex;
        /*
        PG_ASSERT( desc.type == ImageType::TYPE_2D || desc.type == ImageType::TYPE_CUBEMAP, "Currently only support 2D and CUBEMAP images" );
        PG_ASSERT( desc.mipLevels == 1, "Mipmaps not supported yet" );
        PG_ASSERT( desc.srcFormat != PixelFormat::NUM_PIXEL_FORMATS );
        Texture tex;
        tex.m_desc = desc;
        glGenTextures( 1, &tex.m_nativeHandle );
        
        auto nativeTexType                        = PGToOpenGLImageType( desc.type );
        auto [nativePixelFormat, nativePixelType] = PGToOpenGLFormatAndType( desc.srcFormat );
        GLenum dstFormat;
        if ( desc.dstFormat == PixelFormat::NUM_PIXEL_FORMATS )
        {
            dstFormat = PGToOpenGLPixelFormat( desc.srcFormat );
        }
        else
        {
            dstFormat = PGToOpenGLPixelFormat( desc.dstFormat );
        }
        glBindTexture( nativeTexType, tex.m_nativeHandle );

        if ( desc.type == ImageType::TYPE_2D )
        {
            glTexImage2D( nativeTexType, 0, dstFormat, desc.width, desc.height, 0, nativePixelFormat, nativePixelType, data );
        }
        else if ( desc.type == ImageType::TYPE_CUBEMAP )
        {
            for ( int i = 0; i < 6; ++i )
            {
                unsigned char* faceData = &static_cast< unsigned char* >( data )[i * desc.width * desc.height * SizeOfPixelFromat( desc.srcFormat )];
                glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, dstFormat, desc.width, desc.height, 0, nativePixelFormat, nativePixelType, faceData );
            }
        }

        return tex;
        */
    }

    void Texture::Free()
    {
        /*if ( m_nativeHandle != ~0u )
        {
            glDeleteTextures( 1, &m_nativeHandle );
            m_nativeHandle = ~0u;
        }*/
    }

    unsigned char* Texture::GetPixelData() const
    {
        PG_ASSERT( false );
        return nullptr;
        /*
        PG_ASSERT( m_nativeHandle != ~0u );
        int pixelSize   = SizeOfPixelFromat( m_desc.dstFormat );
        uint32_t w      = m_desc.width;
        uint32_t h      = m_desc.height;
        uint32_t d      = m_desc.depth;
        size_t faceSize = w * h * d * pixelSize;
        size_t size     = m_desc.arrayLayers * faceSize;
        unsigned char* cpuData = static_cast< unsigned char* >( malloc( size ) );
        auto [nativePixelFormat, nativePixelType] = PGToOpenGLFormatAndType( m_desc.dstFormat );
        auto nativeTexType = PGToOpenGLImageType( m_desc.type );
        glBindTexture( nativeTexType, m_nativeHandle );

        auto data = cpuData;
        if ( m_desc.type == ImageType::TYPE_CUBEMAP )
        {
            for ( int i = 0; i < m_desc.arrayLayers; ++i )
            {
                glGetTexImage( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, nativePixelFormat, nativePixelType, data );
                data += faceSize;
            }
        }
        else
        {
            glGetTexImage( nativeTexType, 0, nativePixelFormat, nativePixelType, cpuData );
        }
        
        return cpuData;
        */
    }

    ImageType Texture::GetType() const
    {
        return m_desc.type;
    }

    PixelFormat Texture::GetPixelFormat() const
    {
        return m_desc.dstFormat;
    }

    uint8_t Texture::GetMipLevels() const
    {
        return m_desc.mipLevels;
    }

    uint8_t Texture::GetArrayLayers() const
    {
        return m_desc.arrayLayers;
    }

    uint32_t Texture::GetWidth() const
    {
        return m_desc.width;
    }

    uint32_t Texture::GetHeight() const
    {
        return m_desc.height;
    }

    uint32_t Texture::GetDepth() const
    {
        return m_desc.depth;
    }

    /*GLuint Texture::GetNativeHandle() const
    {
        return m_nativeHandle;
    }

    Texture::operator bool() const
    {
        return m_nativeHandle != ~0u;
    }*/


    // ------------- RENDER PASS ----------------//
    RenderPass::~RenderPass()
    {
        /*if ( m_nativeHandle != 0 && m_nativeHandle != ~0u )
        {
            glDeleteFramebuffers( 1, &m_nativeHandle );
        }*/
    }

    RenderPass::RenderPass( RenderPass&& r )
    {
        *this = std::move( r );
    }

    RenderPass& RenderPass::operator=( RenderPass&& r )
    {
        m_desc = std::move( r.m_desc );
        //m_nativeHandle   = std::move( r.m_nativeHandle );
        //r.m_nativeHandle = ~0u;

        return *this;
    }

    void RenderPass::Bind() const
    {
        PG_ASSERT( false );
        /*
        PG_ASSERT( m_nativeHandle != ~0u );
        glBindFramebuffer( GL_FRAMEBUFFER, m_nativeHandle );
        if ( m_desc.colorAttachmentDescriptors.size() )
        {
            if ( m_desc.colorAttachmentDescriptors[0].loadAction == LoadAction::CLEAR )
            {
                auto& c = m_desc.colorAttachmentDescriptors[0].clearColor;
                glClearColor( c.r, c.g, c.b, c.a );
                glClear( GL_COLOR_BUFFER_BIT );
            }
        }

        if ( m_desc.depthAttachmentDescriptor.loadAction == LoadAction::CLEAR )
        {
            glDepthMask( true );
            glClearDepthf( m_desc.depthAttachmentDescriptor.clearValue );
            glClear( GL_DEPTH_BUFFER_BIT );
        }
        */
    }

    RenderPass RenderPass::Create( const RenderPassDescriptor& desc )
    {
        RenderPass pass;
        pass.m_desc = desc;

        return pass;
        /*
        // Check if this is the screen/default framebuffer
        if ( desc.colorAttachmentDescriptors[0].texture == nullptr && desc.depthAttachmentDescriptor.texture == nullptr )
        {
            pass.m_nativeHandle = 0;
            return pass;
        }

        glGenFramebuffers( 1, &pass.m_nativeHandle );
        glBindFramebuffer( GL_FRAMEBUFFER, pass.m_nativeHandle );

        LoadAction loadAction = desc.colorAttachmentDescriptors[0].loadAction;
        glm::vec4 clearColor  = desc.colorAttachmentDescriptors[0].clearColor;

        // Currently am not supporting color attachments to have different clear values
        for ( size_t i = 1; i < desc.colorAttachmentDescriptors.size() && 
              desc.colorAttachmentDescriptors[i].texture != nullptr; ++i )
        {
            if ( desc.colorAttachmentDescriptors[0].loadAction != loadAction ||
                 desc.colorAttachmentDescriptors[0].clearColor != clearColor )
            {
                LOG_ERR( "Currently need all color attachments to have the same clear color and load action" );
                PG_ASSERT( desc.colorAttachmentDescriptors[0].loadAction == loadAction &&
                           desc.colorAttachmentDescriptors[0].clearColor == clearColor );
            }
        }

        if ( desc.depthAttachmentDescriptor.texture )
        {
            auto nativeHandle = desc.depthAttachmentDescriptor.texture->GetNativeHandle();
            glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, nativeHandle, 0 );
        }

        std::vector< GLenum > attachments;
        for ( unsigned int i = 0; i < (unsigned int) desc.colorAttachmentDescriptors.size() && 
              desc.colorAttachmentDescriptors[i].texture != nullptr; ++i )
        {
            GLuint slot = GL_COLOR_ATTACHMENT0 + i ;
            glFramebufferTexture2D( GL_FRAMEBUFFER, slot, GL_TEXTURE_2D,
                                    desc.colorAttachmentDescriptors[i].texture->GetNativeHandle(), 0 );
            attachments.push_back( slot );
        }
        glDrawBuffers( static_cast< int >( attachments.size() ), &attachments[0] );

        if ( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
        {
            LOG_ERR( "Unable to complete the creation of the render pass" );
            return {};
        }

        return pass;
        */
    }

   /* GLuint RenderPass::GetNativeHandle() const
    {
        return m_nativeHandle;
    }*/

    Pipeline Pipeline::Create( const PipelineDescriptor& desc )
    {
        Pipeline p;
        p.m_desc = desc;
        p.m_vertexDesc = VertexInputDescriptor::Create( desc.numVertexDescriptors, desc.vertexDescriptors );

        return p;
    }

    void Pipeline::Bind() const
    {
        /*
        m_vertexDesc.Bind();

        if ( m_desc.depthInfo.depthTestEnabled )
        {
            glEnable( GL_DEPTH_TEST );
        }
        else
        {
            glDisable( GL_DEPTH_TEST );
        }

        glDepthMask( m_desc.depthInfo.depthWriteEnabled );

        auto nativeDepthCompareFunc = PGToOpenGLDepthCompareFunction( m_desc.depthInfo.compareFunc );
        glDepthFunc( nativeDepthCompareFunc );

        auto nativeWindingOrder = PGToOpenGLWindingOrder( m_desc.windingOrder );
        glFrontFace( nativeWindingOrder );

        if ( m_desc.cullFace == CullFace::NONE )
        {
            glDisable( GL_CULL_FACE );
        }
        else
        {
            glEnable( GL_CULL_FACE );
            auto nativeFrontFace = PGToOpenGLCullFace( m_desc.cullFace );
            glCullFace( nativeFrontFace );
        }

        for ( int i = 0; i < m_desc.numColorAttachments; ++i )
        {
            auto& c = m_desc.colorAttachmentInfos[i];
            if ( c.blendingEnabled )
            {
                glEnablei( GL_BLEND, i );
                auto nativeColorEq = PGToOpenGLBlendEquation( c.colorBlendEquation );
                auto nativeAlphaEq = PGToOpenGLBlendEquation( c.alphaBlendEquation );
                glBlendEquationSeparatei( i, nativeColorEq, nativeAlphaEq );

                auto nativeSrcColor = PGToOpenGLBlendFactor( c.srcColorBlendFactor );
                auto nativeDstColor = PGToOpenGLBlendFactor( c.dstColorBlendFactor );
                auto nativeSrcAlpha = PGToOpenGLBlendFactor( c.srcAlphaBlendFactor );
                auto nativeDstAlpha = PGToOpenGLBlendFactor( c.dstAlphaBlendFactor );
                glBlendFuncSeparatei( i, nativeSrcColor, nativeDstColor, nativeSrcAlpha, nativeDstAlpha );
            }
            else
            {
                glDisablei( GL_BLEND, i );
            }
        }
        */
    }

    /*void Blit( const RenderPass& src, const RenderPass& dst, int width, int height,
               const RenderTargetBuffers& mask, FilterMode filter )
    {
        glBindFramebuffer( GL_READ_FRAMEBUFFER, src.GetNativeHandle() );
        glBindFramebuffer( GL_DRAW_FRAMEBUFFER, dst.GetNativeHandle() );
        auto nativeMask   = PGToOpenGLBitFieldMask( mask );
        auto nativeFilter = PGToOpenGLFilterMode( filter );
        glBlitFramebuffer( 0, 0, width, height, 0, 0, width, height, nativeMask, nativeFilter );
    }*/

} // namespace Gfx
} // namespace Progression
