#include "resource/image.hpp"
#include "core/assert.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#include <cstdlib>
#include <filesystem>
#include <utility>

namespace Progression
{

int SizeOfPixelFromat( const PixelFormat& format )
{
    int size[] =
    {
        1,  // R8_Uint
        2,  // R16_Float
        4,  // R32_Float

        2,  // R8_G8_Uint
        4,  // R16_G16_Float
        8,  // R32_G32_Float

        3,  // R8_G8_B8_Uint
        6,  // R16_G16_B16_Float
        12, // R32_G32_B32_Float

        4,  // R8_G8_B8_A8_Uint
        8,  // R16_G16_B16_A16_Float
        16, // R32_G32_B32_A32_Float

        4, // R8_G8_B8_Uint_sRGB
        4, // R8_G8_B8_A8_Uint_sRGB

        4, // R11_G11_B10_Float

        4, // DEPTH32_Float
    };

    PG_ASSERT( static_cast< int >( format ) < static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS ) );
    static_assert( ARRAY_COUNT( size ) == static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS ) );

    return size[static_cast< int >( format )];
}

int NumComponentsInPixelFromat( const PixelFormat& format )
{
    int components[] =
    {
        1,  // R8_Uint
        1,  // R16_Float
        1,  // R32_Float

        2,  // R8_G8_Uint
        2,  // R16_G16_Float
        2,  // R32_G32_Float

        3,  // R8_G8_B8_Uint
        3,  // R16_G16_B16_Float
        3,  // R32_G32_B32_Float

        4,  // R8_G8_B8_A8_Uint
        4,  // R16_G16_B16_A16_Float
        4,  // R32_G32_B32_A32_Float

        4, // R8_G8_B8_Uint_sRGB
        4, // R8_G8_B8_A8_Uint_sRGB

        3, // R11_G11_B10_Float

        1, // DEPTH32_Float
    };

    PG_ASSERT( static_cast< int >( format ) < static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS ) );
    static_assert( ARRAY_COUNT( components ) == static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS ) );

    return components[static_cast< int >( format )];
}

Image::Image( const ImageDesc& desc ) :
    m_desc( desc ),
    m_pixels( nullptr )
{
    m_pixels = (unsigned char*) malloc( GetTotalImageBytes() );
}

Image::~Image()
{
    if ( m_pixels )
    {
        free( m_pixels );
    }
}

Image::Image( Image&& src )
{
    *this = std::move( src );
}

Image& Image::operator=( Image&& src )
{
    if ( m_pixels )
    {
        free( m_pixels );
    }

    m_desc          = src.m_desc;
    m_pixels        = src.m_pixels;
    src.m_pixels    = nullptr;

    return *this;
}

bool Image::Load( const std::string& fname, bool flip_vertically )
{
    std::string ext = std::filesystem::path( fname ).extension();
    if ( ext == ".jpg" || ext == ".png" || ext == ".tga" || ext == ".bmp" )
    {
        stbi_set_flip_vertically_on_load( flip_vertically );
        int width, height, numComponents;
        unsigned char* pixels = stbi_load( fname.c_str(), &width, &height, &numComponents, 0 );

        if ( !pixels )
        {
            LOG_ERR( "Failed to load image '", fname, "'" );
            return false;
        }

        m_pixels            = pixels;
        m_desc.width        = width;
        m_desc.height       = height;
        m_desc.depth        = 1;
        m_desc.arrayLayers  = 1;
        m_desc.mipLevels    = 1;
        m_desc.type         = ImageType::TYPE_2D;
        
        // TODO: how to detect sRGB?
        PixelFormat componentsToFormat[] =
        {
            PixelFormat::R8_Uint,
            PixelFormat::R8_G8_Uint,
            PixelFormat::R8_G8_B8_Uint,
            PixelFormat::R8_G8_B8_A8_Uint,
        };
        m_desc.format = componentsToFormat[numComponents - 1];
    }
    else
    {
        LOG_ERR( "Image filetype '", ext, "' is not supported" );
        return false;
    }

    return true;
}

bool Image::Save( const std::string& fname, bool flipVertically ) const
{
    std::string ext = std::filesystem::path( fname ).extension();
    if ( ext == ".jpg" || ext == ".png" || ext == ".tga" || ext == ".bmp" )
    {
        if ( m_desc.type != ImageType::TYPE_2D )
        {
            LOG_ERR( "Can't save image with multiple faces, mips, or depth > 1 to file format: ", ext );
            return false;
        }

        stbi_flip_vertically_on_write( flipVertically );
        int i = static_cast< int >( fname.length() );
        while ( fname[--i] != '.' && i >= 0 );
        if ( i < 0 )
        {
            LOG_ERR( "Image filename \"", fname, "\" has no extension" );
            return false;
        }

        int numComponents = NumComponentsInPixelFromat( m_desc.format );

        int ret;
        switch ( fname[i + 1] )
        {
            case 'p':
                ret = stbi_write_png( fname.c_str(), m_desc.width, m_desc.height, numComponents, m_pixels,
                                      m_desc.width * numComponents );
                break;
            case 'j':
                ret = stbi_write_jpg( fname.c_str(), m_desc.width, m_desc.height, numComponents, m_pixels, 95 );
                break;
            case 'b':
                ret = stbi_write_bmp( fname.c_str(), m_desc.width, m_desc.height, numComponents, m_pixels );
                break;
            case 't':
                ret = stbi_write_tga( fname.c_str(), m_desc.width, m_desc.height, numComponents, m_pixels );
                break;
            default:
                LOG_ERR( "Cant save an image with an unrecognized format: ", fname );
                return false;
        }
        if ( !ret )
        {
            LOG_ERR( "Failed to write image: ", fname );
            return false;
        }
    }
    else
    {
        LOG_ERR( "Saving image as filetype '", ext, "' is not supported." );
        return false;
    }

    return true;
}

bool Image::Serialize( std::ofstream& out ) const
{
    serialize::Write( out, m_desc.type );
    serialize::Write( out, m_desc.format );
    serialize::Write( out, m_desc.mipLevels );
    serialize::Write( out, m_desc.arrayLayers  );
    serialize::Write( out, m_desc.width );
    serialize::Write( out, m_desc.height );
    serialize::Write( out, m_desc.depth );
    serialize::Write( out, (char*) m_pixels, GetTotalImageBytes() );

    return !out.fail();
}

bool Image::Deserialize( char*& buffer)
{
    serialize::Read( buffer, m_desc.type );
    serialize::Read( buffer, m_desc.format );
    serialize::Read( buffer, m_desc.mipLevels );
    serialize::Read( buffer, m_desc.arrayLayers  );
    serialize::Read( buffer, m_desc.width );
    serialize::Read( buffer, m_desc.height );
    serialize::Read( buffer, m_desc.depth );
    size_t totalSize;
    serialize::Read( buffer, totalSize );
    m_pixels = (unsigned char*) malloc( totalSize );
    serialize::Read( buffer, (char*) m_pixels, totalSize );
    
    return true;
}

ImageDesc Image::GetDescriptor() const
{
    return m_desc;
}

ImageType Image::GetType() const
{
    return m_desc.type;
}

PixelFormat Image::GetPixelFormat() const
{
    return m_desc.format;
}

uint8_t Image::GetMipLevels() const
{
    return m_desc.mipLevels;
}

uint8_t Image::GetArrayLayers() const
{
    return m_desc.arrayLayers;
}

uint32_t Image::GetWidth() const
{
    return m_desc.width;
}

uint32_t Image::GetHeight() const
{
    return m_desc.height;
}

uint32_t Image::GetDepth() const
{
    return m_desc.depth;
}

unsigned char* Image::GetPixels() const
{
    return m_pixels;
}

size_t Image::GetTotalImageBytes() const
{
    PG_ASSERT( m_desc.mipLevels > 0 );
    int pixelSize = SizeOfPixelFromat( m_desc.format );
    uint32_t w    = m_desc.width;
    uint32_t h    = m_desc.width;
    size_t size   = 0;
    for ( uint8_t i = 0; i < m_desc.mipLevels; ++i )
    {
        size += m_desc.arrayLayers * m_desc.depth * w * h * pixelSize;
        w >>= 1;
        h >>= 1;
    }

    return size;
}


} // namespace Progression
