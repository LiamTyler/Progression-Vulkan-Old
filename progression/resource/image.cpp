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

    PG_ASSERT( static_cast< int >( format )   < static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS ) );
    static_assert( ARRAY_COUNT( components ) == static_cast< int >( PixelFormat::NUM_PIXEL_FORMATS ) );

    return components[static_cast< int >( format )];
}

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

bool Image::Load( ResourceCreateInfo* createInfo )
{
    PG_ASSERT( createInfo );
    ImageCreateInfo* info = static_cast< ImageCreateInfo* >( createInfo );
    name                  = info->name;
    m_flags               = info->flags;
    PG_ASSERT( ( m_flags & IMAGE_CREATE_TEXTURE_ON_LOAD ) || !( m_flags & IMAGE_FREE_CPU_COPY_ON_LOAD ) );

    int numImages = static_cast< int >( info->filenames.size() );
    PG_ASSERT( numImages == 1 || numImages == 6 );
    std::vector< ImageDescriptor > imageDescs( numImages );
    std::vector< unsigned char* > imageData( numImages );

    for ( int i = 0; i < numImages; ++i )
    {
        std::string file = info->filenames[i];
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
            
            // TODO: how to detect sRGB?
            PixelFormat componentsToFormat[] =
            {
                PixelFormat::R8_UNORM,
                PixelFormat::R8_G8_UNORM,
                PixelFormat::R8_G8_B8_UNORM,
                PixelFormat::R8_G8_B8_A8_UNORM,
            };
            imageDescs[i].srcFormat = componentsToFormat[numComponents - 1];
            imageDescs[i].dstFormat = componentsToFormat[numComponents - 1];
            LOG( "Image: ", name, ", numComponents: ", numComponents );
        }
        else
        {
            LOG_ERR( "Image filetype '", ext, "' is not supported" );
            return false;
        }
    }

    for ( int i = 1; i < numImages; ++i )
    {
        if ( imageDescs[0].width     != imageDescs[i].width ||
             imageDescs[0].height    != imageDescs[i].height ||
             imageDescs[0].srcFormat != imageDescs[i].srcFormat )
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
        size_t imSize = imageDescs[0].width * imageDescs[0].height * SizeOfPixelFromat( imageDescs[0].srcFormat );
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

    // serialize::Write( out, name );
    // serialize::Write( out, m_flags );
    serialize::Write( out, m_texture.m_desc.type );
    serialize::Write( out, m_texture.m_desc.srcFormat );
    serialize::Write( out, m_texture.m_desc.dstFormat );
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
    std::string samplerName;
    serialize::Read( buffer, samplerName );
    if ( !samplerName.empty() )
    {
        sampler = RenderSystem::GetSampler( samplerName );
        PG_ASSERT( sampler != nullptr, ( "No sampler found with name '" + samplerName + "' found" ).c_str() );
    }
    else
    {
        sampler = nullptr;
    }
    serialize::Read( buffer, m_texture.m_desc.type );
    serialize::Read( buffer, m_texture.m_desc.srcFormat );
    serialize::Read( buffer, m_texture.m_desc.dstFormat );
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

        int numComponents = NumComponentsInPixelFromat( m_texture.m_desc.dstFormat );

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
    void* stagingBufferData = stagingBuffer.Map();
    memcpy( stagingBufferData, m_pixels, imSize );
    stagingBuffer.UnMap();

    VkFormat vkFormat = PGToVulkanPixelFormat( m_texture.GetPixelFormat() );
    PG_ASSERT( FormatSupported( vkFormat, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT ) );

    m_texture = device.NewTexture( m_texture.m_desc );
    TransitionImageLayout( m_texture.GetHandle(), vkFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );
    device.CopyBufferToImage( stagingBuffer, m_texture );
    TransitionImageLayout( m_texture.GetHandle(), vkFormat,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

    stagingBuffer.Free();

    m_texture.m_imageView = CreateImageView( m_texture.m_image, vkFormat );
}

void Image::ReadToCpu()
{
    PG_ASSERT( false, "Currently don't support reading texture data back to the cpu" );
    FreeCpuCopy();
    m_pixels = m_texture.GetPixelData();
    m_texture.m_desc.srcFormat = m_texture.m_desc.dstFormat;
}

void Image::FreeGpuCopy()
{
    m_texture.Free();
}

void Image::FreeCpuCopy()
{
    if ( m_pixels )
    {
        free( m_pixels );
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

PixelFormat Image::GetSrcPixelFormat() const
{
    return m_texture.m_desc.srcFormat;
}

PixelFormat Image::GetDstPixelFormat() const
{
    return m_texture.m_desc.dstFormat;
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
    int pixelSize = SizeOfPixelFromat( m_texture.m_desc.dstFormat );
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
