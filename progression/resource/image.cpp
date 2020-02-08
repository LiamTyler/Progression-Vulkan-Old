#include "resource/image.hpp"
#include "core/assert.hpp"
#include "graphics/debug_marker.hpp"
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

std::shared_ptr< Image > Image::Load2DImageWithDefaultSettings( const std::string& filename, ImageSemantic semantic )
{
    ImageCreateInfo info;
    info.filename = filename;
    info.name     = std::filesystem::path( filename ).stem().string();
    info.flags    = IMAGE_FLIP_VERTICALLY | IMAGE_CREATE_TEXTURE_ON_LOAD | IMAGE_GENERATE_MIPMAPS;
    info.sampler  = "linear_repeat_linear";
    info.semantic = semantic;
    std::shared_ptr< Image > image = std::make_shared< Image >();
    if ( !image->Load( &info ) )
    {
        LOG_ERR( "Could not load image with name '", info.name, "' and filename '", filename, "'" );
        return nullptr;
    }

    return image;
}

static bool LoadSingleImage( const std::string& filename, ImageCreateInfo const * info, ImageDescriptor& desc, unsigned char*& data )
{
    // Force RGB textures to be RGBA since most GPUs don't support RGB optimial
    // texture formats: https://vulkan.gpuinfo.org/
    int width, height, numComponents, forceNumComponents;
    auto ret = stbi_info( filename.c_str(), &width, &height, &numComponents );
    if ( !ret )
    {
        LOG_ERR( "Error opening image '", filename, "'" );
        return false;
    }
    if ( numComponents == 3 )
    {
        forceNumComponents = 4;
    }
    else
    {
        forceNumComponents = numComponents;
    }

    stbi_set_flip_vertically_on_load( info->flags & IMAGE_FLIP_VERTICALLY );
    unsigned char* pixels;
    bool hdrImageFile = stbi_is_hdr( filename.c_str() );
    if ( hdrImageFile )
    {
        pixels = (unsigned char*) stbi_loadf( filename.c_str(), &width, &height, &numComponents, forceNumComponents );
    }
    else
    {
        pixels = stbi_load( filename.c_str(), &width, &height, &numComponents, forceNumComponents );
    }

    if ( !pixels )
    {
        LOG_ERR( "Failed to load image '", filename, "'" );
        return false;
    }

    data             = pixels;
    desc.width       = width;
    desc.height      = height;
    desc.depth       = 1;
    desc.arrayLayers = 1;
    desc.mipLevels   = 1;
    desc.type        = ImageType::TYPE_2D;
    desc.sampler     = info->sampler;
            
    // TODO: how to detect sRGB?
    PixelFormat componentsToFormat[] =
    {
        PixelFormat::R8_UNORM,
        PixelFormat::R8_G8_UNORM,
        PixelFormat::R8_G8_B8_UNORM,
        PixelFormat::R8_G8_B8_A8_UNORM,

        PixelFormat::R8_SRGB,
        PixelFormat::R8_G8_SRGB,
        PixelFormat::R8_G8_B8_SRGB,
        PixelFormat::R8_G8_B8_A8_SRGB,

        PixelFormat::R32_FLOAT,
        PixelFormat::R32_G32_FLOAT,
        PixelFormat::R32_G32_B32_FLOAT,
        PixelFormat::R32_G32_B32_A32_FLOAT,
    };
    int formatIndex = forceNumComponents - 1;
    if ( info->semantic == ImageSemantic::DIFFUSE )
    {
        formatIndex += 4;
    }
    if ( hdrImageFile )
    {
        formatIndex = forceNumComponents - 1 + 8;
    }

    desc.format = componentsToFormat[formatIndex];

    return true;
}

bool Image::Load( ResourceCreateInfo* createInfo )
{
    PG_ASSERT( createInfo );
    ImageCreateInfo* info = static_cast< ImageCreateInfo* >( createInfo );
    name                  = info->name;
    m_flags               = info->flags;
    PG_ASSERT( ( m_flags & IMAGE_CREATE_TEXTURE_ON_LOAD ) || !( m_flags & IMAGE_FREE_CPU_COPY_ON_LOAD ) );
    PG_ASSERT( !( !info->filename.empty() && !info->cubeMapFilenames.empty() ), "Can't specify both a single file and cubemap images to load" );
    PG_ASSERT( !info->filename.empty() || info->cubeMapFilenames.size() == 6, "Must specify single file or 6 cubemap files to load" );

    std::vector< std::string > filenames;
    if ( !info->filename.empty() )
    {
        filenames.push_back( info->filename );
    }
    else
    {
        filenames = info->cubeMapFilenames;
    }
    int numImages = static_cast< int >( filenames.size() );
    std::vector< ImageDescriptor > imageDescs( numImages );
    std::vector< unsigned char* > imageData( numImages );

    for ( int i = 0; i < numImages; ++i )
    {
        if ( !LoadSingleImage( filenames[i], info, imageDescs[i], imageData[i] ) )
        {
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
    serialize::Write( out, m_texture.m_desc.arrayLayers );
    serialize::Write( out, m_texture.m_desc.width );
    serialize::Write( out, m_texture.m_desc.height );
    serialize::Write( out, m_texture.m_desc.depth );

    // Can't read back mips yet to cpu, so just save the first mip
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
    auto& device  = g_renderState.device;
    size_t imSize = CalculateTotalTextureSize( m_texture.m_desc );
    Buffer stagingBuffer = device.NewBuffer( imSize, BUFFER_TYPE_TRANSFER_SRC, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT );
    stagingBuffer.Map();
    memcpy( stagingBuffer.MappedPtr(), m_pixels, imSize );
    stagingBuffer.UnMap();

    VkFormat vkFormat = PGToVulkanPixelFormat( m_texture.GetPixelFormat() );
    PG_ASSERT( FormatSupported( vkFormat, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT ) );

    bool generateMips = (m_flags & IMAGE_GENERATE_MIPMAPS) && GetMipLevels() == 1;
    if ( generateMips )
    {
        m_texture.m_desc.mipLevels = static_cast< uint32_t >( 1 + std::floor( std::log2( std::max( m_texture.m_desc.width, m_texture.m_desc.height ) ) ) );
    }

    bool isTex2D = m_texture.m_desc.arrayLayers == 1;
    m_texture = device.NewTexture( m_texture.m_desc, isTex2D, name );
    TransitionImageLayout( m_texture.GetHandle(), vkFormat, VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_texture.m_desc.mipLevels, m_texture.m_desc.arrayLayers );
    
    device.CopyBufferToImage( stagingBuffer, m_texture, !generateMips || Gfx::PixelFormatIsCompressed( GetPixelFormat() ) );

    if ( generateMips )
    {
        m_texture.GenerateMipMaps();
        //PG_ASSERT( ( m_flags & IMAGE_FREE_CPU_COPY_ON_LOAD ), "Mipmaps currently not copied back to the cpu yet" );
    }
    else
    {
        TransitionImageLayout( m_texture.GetHandle(), vkFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_texture.m_desc.mipLevels, m_texture.m_desc.arrayLayers );
    }

    stagingBuffer.Free();
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

uint32_t Image::GetMipLevels() const
{
    return m_texture.m_desc.mipLevels;
}

uint32_t Image::GetArrayLayers() const
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
    return CalculateTotalTextureSize( m_texture.m_desc );
}

ImageFlags Image::GetImageFlags() const
{
    return m_flags;
}

} // namespace Progression
