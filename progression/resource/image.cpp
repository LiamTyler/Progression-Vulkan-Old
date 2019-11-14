#include "resource/image.hpp"
#include "core/assert.hpp"
#include "graphics/render_system.hpp"
#include "graphics/pg_to_vulkan_types.hpp"
#include "graphics/vulkan.hpp"
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

using namespace Gfx;

Image::Image( const ImageDescriptor& desc )
{
    m_texture.m_desc = desc;
    m_pixels         = (unsigned char*) malloc( GetTotalImageBytes() );
}

Image::~Image()
{
    if ( m_pixels )
    {
        free( m_pixels );
    }
    if ( m_texture )
    {
        m_texture.Free();
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

    m_texture       = std::move( src.m_texture );
    m_pixels        = src.m_pixels;
    src.m_pixels    = nullptr;

    return *this;
}

std::shared_ptr< Image > Image::Load2DImageWithDefaultSettings( const std::string& filename )
{
    ImageCreateInfo info;
    info.filename = filename;
    info.name     = std::filesystem::path( filename ).stem().string();
    info.flags    = IMAGE_FLIP_VERTICALLY | IMAGE_CREATE_TEXTURE_ON_LOAD;
    info.sampler  = "linear_repeat";
    std::shared_ptr< Image > image = std::make_shared< Image >();
    if ( !image->Load( &info ) )
    {
        LOG_ERR( "Could not load image with name '", info.name, "' and filename '", filename, "'" );
        return nullptr;
    }

    return image;
}

bool Image::Load( ResourceCreateInfo* createInfo )
{
    PG_ASSERT( createInfo );
    ImageCreateInfo* info = static_cast< ImageCreateInfo* >( createInfo );
    name                  = info->name;
    m_flags               = info->flags;
    PG_ASSERT( ( m_flags & IMAGE_CREATE_TEXTURE_ON_LOAD ) || !( m_flags & IMAGE_FREE_CPU_COPY_ON_LOAD ) );
    PG_ASSERT( !( !info->filename.empty() && !info->skyboxFilenames.empty() ), "Can't specify both a single file and skybox images to load" );
    PG_ASSERT( !info->filename.empty() || info->skyboxFilenames.size() == 6, "Must specify single file or 6 skybox files to load" );

    std::vector< std::string > filenames;
    if ( !info->filename.empty() )
    {
        filenames.push_back( info->filename );
    }
    else
    {
        filenames = info->skyboxFilenames;
    }
    int numImages = static_cast< int >( filenames.size() );
    std::vector< ImageDescriptor > imageDescs( numImages );
    std::vector< unsigned char* > imageData( numImages );

    for ( int i = 0; i < numImages; ++i )
    {
        std::string file = filenames[i];
        std::string ext = std::filesystem::path( file ).extension().string();
        if ( ext == ".jpg" || ext == ".png" || ext == ".tga" || ext == ".bmp" )
        {
            stbi_set_flip_vertically_on_load( info->flags & IMAGE_FLIP_VERTICALLY );
            int width, height, numComponents;
            unsigned char* pixels = stbi_load( file.c_str(), &width, &height, &numComponents, 4 );
            numComponents = 4;

            if ( !pixels )
            {
                LOG_ERR( "Failed to load image '", file, "'" );
                return false;
            }

            imageData[i]              = pixels;
            imageDescs[i].width       = width;
            imageDescs[i].height      = height;
            imageDescs[i].depth       = 1;
            imageDescs[i].arrayLayers = 1;
            imageDescs[i].mipLevels   = 1;
            imageDescs[i].type        = ImageType::TYPE_2D;
            imageDescs[i].sampler     = info->sampler;
            
            // TODO: how to detect sRGB?
            PixelFormat componentsToFormat[] =
            {
                PixelFormat::R8_UNORM,
                PixelFormat::R8_G8_UNORM,
                PixelFormat::R8_G8_B8_UNORM,
                PixelFormat::R8_G8_B8_A8_UNORM,
            };
            imageDescs[i].format = componentsToFormat[numComponents - 1];
        }
        else
        {
            LOG_ERR( "Image filetype '", ext, "' is not supported" );
            return false;
        }
    }

    for ( int i = 1; i < numImages; ++i )
    {
        if ( imageDescs[0].width  != imageDescs[i].width ||
             imageDescs[0].height != imageDescs[i].height ||
             imageDescs[0].format != imageDescs[i].format )
        {
            LOG_ERR( "Skybox images must have the same dimensions and format" );
            return false;
        }
    }

    m_texture.m_desc = imageDescs[0];
    if ( numImages == 1 )
    {
        m_pixels = imageData[0];
    }
    else
    {
        m_texture.m_desc.type        = ImageType::TYPE_CUBEMAP;
        m_texture.m_desc.arrayLayers = 6;
        size_t imSize = imageDescs[0].width * imageDescs[0].height * SizeOfPixelFromat( imageDescs[0].format );
        m_pixels = static_cast< unsigned char* >( malloc( 6 * imSize ) );
        for ( int i = 0; i < numImages; ++i )
        {
            memcpy( m_pixels + i * imSize, imageData[i], imSize );
        }
    }

    if ( m_flags & IMAGE_CREATE_TEXTURE_ON_LOAD )
    {
        UploadToGpu();
    }

    if ( m_flags & IMAGE_FREE_CPU_COPY_ON_LOAD )
    {
        free( m_pixels );
        m_pixels = nullptr;
    }

    return true;
}

void Image::Move( std::shared_ptr< Resource > dst )
{
    PG_ASSERT( std::dynamic_pointer_cast< Image >( dst ) );
    Image* dstPtr = (Image*) dst.get();
    *dstPtr       = std::move( *this );
}

bool Image::Serialize( std::ofstream& out ) const
{
    PG_ASSERT( m_pixels, "Currently need CPU copy of image data to serialize" );

    serialize::Write( out, m_texture.m_desc.type );
    serialize::Write( out, m_texture.m_desc.format );
    serialize::Write( out, m_texture.m_desc.mipLevels );
    serialize::Write( out, m_texture.m_desc.arrayLayers  );
    serialize::Write( out, m_texture.m_desc.width );
    serialize::Write( out, m_texture.m_desc.height );
    serialize::Write( out, m_texture.m_desc.depth );
    size_t totalSize = GetTotalImageBytes();
    serialize::Write( out, totalSize );
    serialize::Write( out, (char*) m_pixels, totalSize );

    return !out.fail();
}

bool Image::Deserialize( char*& buffer )
{
    serialize::Read( buffer, name );
    serialize::Read( buffer, m_flags );
    serialize::Read( buffer, m_texture.m_desc.sampler );
    serialize::Read( buffer, m_texture.m_desc.type );
    serialize::Read( buffer, m_texture.m_desc.format );
    serialize::Read( buffer, m_texture.m_desc.mipLevels );
    serialize::Read( buffer, m_texture.m_desc.arrayLayers  );
    serialize::Read( buffer, m_texture.m_desc.width );
    serialize::Read( buffer, m_texture.m_desc.height );
    serialize::Read( buffer, m_texture.m_desc.depth );
    size_t totalSize;
    serialize::Read( buffer, totalSize );

    if ( !( m_flags & IMAGE_FREE_CPU_COPY_ON_LOAD ) )
    {
        m_pixels = static_cast< unsigned char* >( malloc( totalSize ) );
        serialize::Read( buffer, (char*) m_pixels, totalSize );
    }
    else
    {
        m_pixels = nullptr;
    }

    if ( m_flags & IMAGE_CREATE_TEXTURE_ON_LOAD )
    {
        if ( m_flags & IMAGE_FREE_CPU_COPY_ON_LOAD )
        {
            m_pixels = reinterpret_cast< unsigned char* >( buffer );
            buffer += totalSize;
        }
        UploadToGpu();

        if ( m_flags & IMAGE_FREE_CPU_COPY_ON_LOAD )
        {
            m_pixels = nullptr;
        }
    }
    
    return true;
}

bool Image::Save( const std::string& fname, bool flipVertically ) const
{
    std::string ext = std::filesystem::path( fname ).extension().string();
    if ( ext == ".jpg" || ext == ".png" || ext == ".tga" || ext == ".bmp" )
    {
        if ( m_texture.m_desc.type != ImageType::TYPE_2D )
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

        int numComponents = NumComponentsInPixelFromat( m_texture.m_desc.format );

        int ret;
        switch ( fname[i + 1] )
        {
            case 'p':
                ret = stbi_write_png( fname.c_str(), m_texture.m_desc.width, m_texture.m_desc.height, numComponents, m_pixels,
                                      m_texture.m_desc.width * numComponents );
                break;
            case 'j':
                ret = stbi_write_jpg( fname.c_str(), m_texture.m_desc.width, m_texture.m_desc.height, numComponents, m_pixels, 95 );
                break;
            case 'b':
                ret = stbi_write_bmp( fname.c_str(), m_texture.m_desc.width, m_texture.m_desc.height, numComponents, m_pixels );
                break;
            case 't':
                ret = stbi_write_tga( fname.c_str(), m_texture.m_desc.width, m_texture.m_desc.height, numComponents, m_pixels );
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

void Image::UploadToGpu()
{
    auto& device = g_renderState.device;
    size_t imSize = GetTotalImageBytes();
    Buffer stagingBuffer = device.NewBuffer( imSize, BUFFER_TYPE_TRANSFER_SRC, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT );
    stagingBuffer.Map();
    memcpy( stagingBuffer.MappedPtr(), m_pixels, imSize );
    stagingBuffer.UnMap();

    VkFormat vkFormat = PGToVulkanPixelFormat( m_texture.GetPixelFormat() );
    PG_ASSERT( FormatSupported( vkFormat, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT ) );

    m_texture = device.NewTexture( m_texture.m_desc );
    TransitionImageLayout( m_texture.GetHandle(), vkFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
    device.CopyBufferToImage( stagingBuffer, m_texture );
    TransitionImageLayout( m_texture.GetHandle(), vkFormat,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

    stagingBuffer.Free();

    // m_texture.m_imageView = CreateImageView( m_texture.m_image, vkFormat );
}

void Image::ReadToCpu()
{
    PG_ASSERT( false, "Currently don't support reading texture data back to the cpu" );
    FreeCpuCopy();
    m_pixels = m_texture.GetPixelData();
    m_texture.m_desc.format = m_texture.m_desc.format;
}

void Image::FreeGpuCopy()
{
    if ( m_texture )
    {
        m_texture.Free();
    }
}

void Image::FreeCpuCopy()
{
    if ( m_pixels )
    {
        free( m_pixels );
        m_pixels = nullptr;
    }
}

Texture* Image::GetTexture()
{
    return &m_texture;
}

ImageDescriptor Image::GetDescriptor() const
{
    return m_texture.m_desc;
}

ImageType Image::GetType() const
{
    return m_texture.m_desc.type;
}

PixelFormat Image::GetPixelFormat() const
{
    return m_texture.m_desc.format;
}

uint8_t Image::GetMipLevels() const
{
    return m_texture.m_desc.mipLevels;
}

uint8_t Image::GetArrayLayers() const
{
    return m_texture.m_desc.arrayLayers;
}

uint32_t Image::GetWidth() const
{
    return m_texture.m_desc.width;
}

uint32_t Image::GetHeight() const
{
    return m_texture.m_desc.height;
}

uint32_t Image::GetDepth() const
{
    return m_texture.m_desc.depth;
}

unsigned char* Image::GetPixels() const
{
    return m_pixels;
}

size_t Image::GetTotalImageBytes() const
{
    PG_ASSERT( m_texture.m_desc.mipLevels == 1, "havent added mipmapping yet" );
    int pixelSize = SizeOfPixelFromat( m_texture.m_desc.format );
    uint32_t w    = m_texture.m_desc.width;
    uint32_t h    = m_texture.m_desc.height;
    uint32_t d    = m_texture.m_desc.depth;
    size_t size   = w * h * d * m_texture.m_desc.arrayLayers * pixelSize;

    return size;
}

ImageFlags Image::GetImageFlags() const
{
    return m_flags;
}

} // namespace Progression
