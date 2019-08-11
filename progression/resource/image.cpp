#include "resource/image.hpp"
#include "utils/logger.hpp"
#include <cstdlib>
#include <utility>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"
#include "utils/serialize.hpp"

namespace Progression
{

Image::Image( int w, int h, int nc, unsigned char* pixels ) :
  m_width( w ), m_height( h ), m_numComponents( nc ), m_pixels( pixels )
{
    if ( !m_pixels )
        m_pixels = (unsigned char*) malloc( nc * w * h );
}

Image::~Image()
{
    if ( m_pixels )
        free( m_pixels );
}

Image::Image( const Image& src )
{
    *this = src;
}

Image& Image::operator=( const Image& image )
{
    if ( m_pixels )
        free( m_pixels );

    if ( !image.m_pixels )
    {
        m_pixels        = nullptr;
        m_width         = 0;
        m_height        = 0;
        m_numComponents = 0;
    }
    else
    {
        m_width         = image.m_width;
        m_height        = image.m_height;
        m_numComponents = image.m_numComponents;
        m_pixels        = (unsigned char*) malloc( m_numComponents * m_width * m_height );
        memcpy( m_pixels, image.m_pixels, m_numComponents * m_width * m_height );
    }

    return *this;
}

Image::Image( Image&& src )
{
    *this = std::move( src );
}

Image& Image::operator=( Image&& src )
{
    if ( m_pixels )
        free( m_pixels );
    m_width         = src.m_width;
    m_height        = src.m_height;
    m_numComponents = src.m_numComponents;
    m_pixels        = src.m_pixels;
    src.m_pixels    = nullptr;
    return *this;
}

bool Image::Load( const std::string& fname, bool flip_vertically )
{
    stbi_set_flip_vertically_on_load( flip_vertically );
    int width, height, numComponents;
    unsigned char* pixels = stbi_load( fname.c_str(), &width, &height, &numComponents, 0 );

    if ( !pixels )
    {
        LOG_ERR( "Failed to load image '", fname, "'" );
        return false;
    }
    *this = std::move( Image( width, height, numComponents, pixels ) );

    return true;
}

bool Image::Save( const std::string& fname, bool flipVertically ) const
{
    stbi_flip_vertically_on_write( flipVertically );
    int i = fname.length();
    while ( fname[--i] != '.' && i >= 0 )
        ;
    if ( i < 0 )
    {
        LOG_ERR( "Image filename \"", fname, "\" has no extension" );
        return false;
    }

    int ret;
    switch ( fname[i + 1] )
    {
        case 'p':
            ret = stbi_write_png( fname.c_str(), m_width, m_height, m_numComponents, m_pixels,
                                  m_width * m_numComponents );
            break;
        case 'j':
            ret = stbi_write_jpg( fname.c_str(), m_width, m_height, m_numComponents, m_pixels, 95 );
            break;
        case 'b':
            ret = stbi_write_bmp( fname.c_str(), m_width, m_height, m_numComponents, m_pixels );
            break;
        case 't':
            ret = stbi_write_tga( fname.c_str(), m_width, m_height, m_numComponents, m_pixels );
            break;
        default:
            LOG_ERR( "Cant save an image with an unrecognized format:", fname );
            return false;
    }
    if ( !ret )
        LOG_ERR( "failed to write image:", fname );

    return ret;
}

bool Image::Serialize( std::ofstream& out ) const
{
    serialize::Write( out, m_width );
    serialize::Write( out, m_height );
    serialize::Write( out, m_numComponents );
    serialize::Write( out, (char*) m_pixels, m_width * m_height * m_numComponents );
    return !out.fail();
}

bool Image::Deserialize( std::ifstream& in )
{
    serialize::Read( in, m_width );
    serialize::Read( in, m_height );
    serialize::Read( in, m_numComponents );
    m_pixels = (unsigned char*) malloc( m_width * m_height * m_numComponents );
    serialize::Read( in, (char*) m_pixels, m_width * m_height * m_numComponents );
    return !in.fail();
}

bool Image::Deserialize2( char*& buffer)
{
    serialize::Read( buffer, m_width );
    serialize::Read( buffer, m_height );
    serialize::Read( buffer, m_numComponents );
    m_pixels = (unsigned char*) malloc( m_width * m_height * m_numComponents );
    serialize::Read( buffer, (char*) m_pixels, m_width * m_height * m_numComponents );
    
    return true;
}

int Image::Width() const
{
    return m_width;
}

int Image::Height() const
{
    return m_height;
}

int Image::NumComponents() const
{
    return m_numComponents;
}

unsigned char* Image::Pixels() const
{
    return m_pixels;
}

} // namespace Progression
