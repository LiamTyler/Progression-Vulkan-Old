#include "graphics/graphics_api/texture.hpp"
#include "core/assert.hpp"
#include "graphics/vulkan.hpp"
#include "graphics/texture_manager.hpp"

namespace Progression
{
namespace Gfx
{
  
    int SizeOfPixelFromat( const PixelFormat& format )
    {
        int size[] =
        {
            0,  // INVALID

            1,  // R8_UNORM
            1,  // R8_SNORM
            1,  // R8_UINT
            1,  // R8_SINT
            1,  // R8_SRGB

            2,  // R8_G8_UNORM
            2,  // R8_G8_SNORM
            2,  // R8_G8_UINT
            2,  // R8_G8_SINT
            2,  // R8_G8_SRGB

            3,  // R8_G8_B8_UNORM
            3,  // R8_G8_B8_SNORM
            3,  // R8_G8_B8_UINT
            3,  // R8_G8_B8_SINT
            3,  // R8_G8_B8_SRGB

            3,  // B8_G8_R8_UNORM
            3,  // B8_G8_R8_SNORM
            3,  // B8_G8_R8_UINT
            3,  // B8_G8_R8_SINT
            3,  // B8_G8_R8_SRGB

            4,  // R8_G8_B8_A8_UNORM
            4,  // R8_G8_B8_A8_SNORM
            4,  // R8_G8_B8_A8_UINT
            4,  // R8_G8_B8_A8_SINT
            4,  // R8_G8_B8_A8_SRGB

            4,  // B8_G8_R8_A8_UNORM
            4,  // B8_G8_R8_A8_SNORM
            4,  // B8_G8_R8_A8_UINT
            4,  // B8_G8_R8_A8_SINT
            4,  // B8_G8_R8_A8_SRGB

            2,  // R16_UNORM
            2,  // R16_SNORM
            2,  // R16_UINT
            2,  // R16_SINT
            2,  // R16_FLOAT

            4,  // R16_G16_UNORM
            4,  // R16_G16_SNORM
            4,  // R16_G16_UINT
            4,  // R16_G16_SINT
            4,  // R16_G16_FLOAT

            6,  // R16_G16_B16_UNORM
            6,  // R16_G16_B16_SNORM
            6,  // R16_G16_B16_UINT
            6,  // R16_G16_B16_SINT
            6,  // R16_G16_B16_FLOAT

            8,  // R16_G16_B16_A16_UNORM
            8,  // R16_G16_B16_A16_SNORM
            8,  // R16_G16_B16_A16_UINT
            8,  // R16_G16_B16_A16_SINT
            8,  // R16_G16_B16_A16_FLOAT

            4,  // R32_UINT
            4,  // R32_SINT
            4,  // R32_FLOAT

            8,  // R32_G32_UINT
            8,  // R32_G32_SINT
            8,  // R32_G32_FLOAT

            12, // R32_G32_B32_UINT
            12, // R32_G32_B32_SINT
            12, // R32_G32_B32_FLOAT

            16, // R32_G32_B32_A32_UINT
            16, // R32_G32_B32_A32_SINT
            16, // R32_G32_B32_A32_FLOAT

            2,  // DEPTH_16_UNORM
            4,  // DEPTH_32_FLOAT
            3,  // DEPTH_16_UNORM_STENCIL_8_UINT
            4,  // DEPTH_24_UNORM_STENCIL_8_UINT
            5,  // DEPTH_32_FLOAT_STENCIL_8_UINT

            1,  // STENCIL_8_UINT

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

    int NumComponentsInPixelFromat( const PixelFormat& format )
    {
        int components[] =
        {
            0,  // INVALID
            1,  // R8_UNORM
            1,  // R8_SNORM
            1,  // R8_UINT
            1,  // R8_SINT
            1,  // R8_SRGB
            2,  // R8_G8_UNORM
            2,  // R8_G8_SNORM
            2,  // R8_G8_UINT
            2,  // R8_G8_SINT
            2,  // R8_G8_SRGB
            3,  // R8_G8_B8_UNORM
            3,  // R8_G8_B8_SNORM
            3,  // R8_G8_B8_UINT
            3,  // R8_G8_B8_SINT
            3,  // R8_G8_B8_SRGB
            3,  // B8_G8_R8_UNORM
            3,  // B8_G8_R8_SNORM
            3,  // B8_G8_R8_UINT
            3,  // B8_G8_R8_SINT
            3,  // B8_G8_R8_SRGB
            4,  // R8_G8_B8_A8_UNORM
            4,  // R8_G8_B8_A8_SNORM
            4,  // R8_G8_B8_A8_UINT
            4,  // R8_G8_B8_A8_SINT
            4,  // R8_G8_B8_A8_SRGB
            4,  // B8_G8_R8_A8_UNORM
            4,  // B8_G8_R8_A8_SNORM
            4,  // B8_G8_R8_A8_UINT
            4,  // B8_G8_R8_A8_SINT
            4,  // B8_G8_R8_A8_SRGB
            1,  // R16_UNORM
            1,  // R16_SNORM
            1,  // R16_UINT
            1,  // R16_SINT
            1,  // R16_FLOAT
            2,  // R16_G16_UNORM
            2,  // R16_G16_SNORM
            2,  // R16_G16_UINT
            2,  // R16_G16_SINT
            2,  // R16_G16_FLOAT
            3,  // R16_G16_B16_UNORM
            3,  // R16_G16_B16_SNORM
            3,  // R16_G16_B16_UINT
            3,  // R16_G16_B16_SINT
            3,  // R16_G16_B16_FLOAT
            4,  // R16_G16_B16_A16_UNORM
            4,  // R16_G16_B16_A16_SNORM
            4,  // R16_G16_B16_A16_UINT
            4,  // R16_G16_B16_A16_SINT
            4,  // R16_G16_B16_A16_FLOAT
            1,  // R32_UINT
            1,  // R32_SINT
            1,  // R32_FLOAT
            2,  // R32_G32_UINT
            2,  // R32_G32_SINT
            2,  // R32_G32_FLOAT
            3,  // R32_G32_B32_UINT
            3,  // R32_G32_B32_SINT
            3,  // R32_G32_B32_FLOAT
            4,  // R32_G32_B32_A32_UINT
            4,  // R32_G32_B32_A32_SINT
            4,  // R32_G32_B32_A32_FLOAT
            1,  // DEPTH_16_UNORM
            1,  // DEPTH_32_FLOAT
            2,  // DEPTH_16_UNORM_STENCIL_8_UINT
            2,  // DEPTH_24_UNORM_STENCIL_8_UINT
            2,  // DEPTH_32_FLOAT_STENCIL_8_UINT
            1,  // STENCIL_8_UINT
            3,  // BC1_RGB_UNORM
            3,  // BC1_RGB_SRGB
            4,  // BC1_RGBA_UNORM
            4,  // BC1_RGBA_SRGB
            2,  // BC2_UNORM
            4,  // BC2_SRGB
            4,  // BC3_UNORM
            4,  // BC3_SRGB
            1,  // BC4_UNORM
            1,  // BC4_SNORM
            2,  // BC5_UNORM
            2,  // BC5_SNORM
            3,  // BC6H_UFLOAT
            3,  // BC6H_SFLOAT
            3,  // BC7_UNORM (can be RGB or RGBA)
            3,  // BC7_SRGB  (can be RGB or RGBA)
        };

        PG_ASSERT( static_cast< int >( format )   < static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS ) );
        static_assert( ARRAY_COUNT( components ) == static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS ) );

        return components[static_cast< int >( format )];
    }

    bool PixelFormatIsCompressed( const PixelFormat& format )
    {
        int f = static_cast< int >( format );
        return static_cast< int >( PixelFormat::BC1_RGB_UNORM ) <= f && f < static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS );
    }

    bool PixelFormatIsDepthFormat( const PixelFormat& format )
    {
        int f = static_cast< int >( format );
        return static_cast< int >( PixelFormat::DEPTH_16_UNORM ) <= f &&
               f <= static_cast< int >( PixelFormat::DEPTH_32_FLOAT_STENCIL_8_UINT );
    }

    bool PixelFormatIsStencil( const PixelFormat& format )
    {
        int f = static_cast< int >( format );
        return static_cast< int >( PixelFormat::DEPTH_16_UNORM_STENCIL_8_UINT ) <= f &&
               f <= static_cast< int >( PixelFormat::STENCIL_8_UINT );
    }

    void Texture::Free()
    {
        PG_ASSERT( m_image     != VK_NULL_HANDLE );
        PG_ASSERT( m_imageView != VK_NULL_HANDLE );
        PG_ASSERT( m_memory    != VK_NULL_HANDLE );

        vkDestroyImage( m_device, m_image, nullptr );
        vkDestroyImageView( m_device, m_imageView, nullptr );
        vkFreeMemory( m_device, m_memory, nullptr );
        if ( m_textureSlot != PG_INVALID_TEXTURE_INDEX )
        {
            FreeTextureSlot( m_textureSlot );
            m_textureSlot = PG_INVALID_TEXTURE_INDEX;
        }
        m_image       = VK_NULL_HANDLE;
        m_imageView   = VK_NULL_HANDLE;
        m_memory      = VK_NULL_HANDLE;
    }

    unsigned char* Texture::GetPixelData() const
    {
        PG_ASSERT( false );
        return nullptr;
    }

    ImageType Texture::GetType() const
    {
        return m_desc.type;
    }

    PixelFormat Texture::GetPixelFormat() const
    {
        return m_desc.format;
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

    VkImage Texture::GetHandle() const
    {
        return m_image;
    }

    VkImageView Texture::GetView() const
    {
        return m_imageView;
    }

    uint16_t Texture::GetShaderSlot() const
    {
        return m_textureSlot;
    }

    Texture::operator bool() const
    {
        return m_image != VK_NULL_HANDLE;
    }

} // namespace Gfx
} // namespace Progression
