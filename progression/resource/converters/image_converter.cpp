#include "resource/converters/image_converter.hpp"
#include "basis_universal/transcoder/basisu_transcoder.h"
#include "basis_universal/basisu_comp.h"
#include "core/assert.hpp"
#include "graphics/render_system.hpp"
#include "resource/image.hpp"
#include "resource/resource_manager.hpp"
#include "resource/resource_version_numbers.hpp"
#include "utils/fileIO.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#include "utils/string.hpp"
#include "utils/timestamp.hpp"
#include <filesystem>
#include <fstream>
#include <thread>

using namespace Progression;
namespace fs = std::filesystem;

static bool CompressAndTranscodeImage( const Image& uncompressedImage, Image& outputCompressedImage, const ImageCreateInfo& createInfo );

static std::string ImageCompressionQualityToString( ImageCompressionQuality q )
{
    std::string names[] =
    {
        "LOW",
        "MEDIUM",
        "HIGH",
        "MAX",
    };

    return names[static_cast< int >( q )];
}

static std::string GetContentFastFileName( ImageCreateInfo& createInfo )
{
    std::string hash, baseName;
    if ( !createInfo.filename.empty() )
    {
        fs::path filePath = fs::absolute( createInfo.filename );
        hash     = std::to_string( std::hash< std::string >{}( filePath.string() ) );
        baseName = filePath.filename().string();
    }
    else
    {
        fs::path filePath = fs::absolute( createInfo.cubeMapFilenames[0] );
        hash     = std::to_string( std::hash< std::string >{}( filePath.string() ) );
        baseName = "skybox_" + filePath.filename().string();
    }
    baseName = SlashesToUnderscores( baseName );
    
    std::string flip      = createInfo.flags & IMAGE_FLIP_VERTICALLY ? "1" : "0";
    std::string version   = std::to_string( PG_RESOURCE_IMAGE_VERSION );
    std::string dstFormat = Gfx::PixelFormatName( createInfo.dstFormat );

    if ( Gfx::PixelFormatIsCompressed( createInfo.dstFormat ) )
    {
        std::string quality = ImageCompressionQualityToString( createInfo.compressionQuality );
        return PG_RESOURCE_DIR "cache/images/" + baseName + "_" + dstFormat + "_" + quality + "_" + flip + "_" + version + "_" + hash + ".ffi";
    }
    else
    {
        return PG_RESOURCE_DIR "cache/images/" + baseName + "_" + dstFormat + "_" + flip + "_" + version + "_" + hash + ".ffi";
    }
}

static std::string GetSettingsFastFileName( const ImageCreateInfo& createInfo )
{
    std::string n = SlashesToUnderscores( createInfo.name );
    return PG_RESOURCE_DIR "cache/images/settings_" + n + "_" + createInfo.sampler + "_" +
           std::to_string( static_cast< int >( createInfo.flags ) ) + ".ffi";
}

ImageConverter::ImageConverter( bool f, bool v )
{
    force              = f;
    verbose            = v;
    createInfo.flags   = IMAGE_FLIP_VERTICALLY | IMAGE_CREATE_TEXTURE_ON_LOAD | IMAGE_FREE_CPU_COPY_ON_LOAD;
    createInfo.sampler = "linear_repeat_linear";
}

AssetStatus ImageConverter::CheckDependencies()
{
    PG_ASSERT( !createInfo.filename.empty() || !createInfo.cubeMapFilenames.empty() );

    // Add an empty image to the manager to notify other resources that an image with this
    // name has already been loaded
    auto placeHolderImg  = std::make_shared< Image >();
    placeHolderImg->name = createInfo.name;
    ResourceManager::Add< Image >( placeHolderImg );
    if ( !RenderSystem::GetSampler( createInfo.sampler ) )
    {
        LOG_ERR( "Sampler '", createInfo.sampler, "' does not exist in the render system" );
        return ASSET_CHECKING_ERROR;
    }

    m_outputContentFile  = GetContentFastFileName( createInfo );
    m_outputSettingsFile = GetSettingsFastFileName( createInfo );

    IF_VERBOSE_MODE( LOG( "\nImage with name '", createInfo.name, "' outputs:" ) );
    IF_VERBOSE_MODE( LOG( "\tContentFile: ", m_outputContentFile ) );
    IF_VERBOSE_MODE( LOG( "\tSettingsFile: ", m_outputSettingsFile ) );

    if ( !std::filesystem::exists( m_outputSettingsFile ) )
    {
        LOG( "OUT OF DATE: FFI File for image settings file '", m_outputSettingsFile, "' does not exist, needs to be generated" );
        m_settingsNeedsConverting = true;
    }
    else
    {
        m_settingsNeedsConverting = false;
    }

    if ( !std::filesystem::exists( m_outputContentFile ) )
    {
        LOG( "OUT OF DATE: FFI File for image '", createInfo.name, "' does not exist, convert required" );
        m_contentNeedsConverting = true;
    }
    else
    {
        auto timestamp = Timestamp( m_outputContentFile );
        IF_VERBOSE_MODE( LOG( "Image FFI timestamp: ", timestamp ) );
        for ( const auto& fname : createInfo.cubeMapFilenames )
        {
            auto ts = Timestamp( fname ) ;
            if ( timestamp < ts )
            {
                IF_VERBOSE_MODE( LOG( "Input image '", fname, ", timestamp: ", ts ) );
                LOG( "OUT OF DATE: Image '", fname, "' has newer timestamp than saved FFI" );
                m_contentNeedsConverting = true;
                status = ASSET_OUT_OF_DATE;
                return status;
            }
        }
        m_contentNeedsConverting = false;
    }

    if ( m_settingsNeedsConverting || m_contentNeedsConverting )
    {
        status = ASSET_OUT_OF_DATE;
    }
    else
    {
        if ( force )
        {
            LOG( "UP TO DATE: Image with name '", createInfo.name, "', but --force used, so converting anyways" );
            status = ASSET_OUT_OF_DATE;
        }
        else
        {
            status = ASSET_UP_TO_DATE;
            LOG( "UP TO DATE: Image with name '", createInfo.name, "'" );
        }
    }

    return status;
}

ConverterStatus ImageConverter::Convert()
{
    if ( status == ASSET_UP_TO_DATE )
    {
        return CONVERT_SUCCESS;
    }

    if ( m_settingsNeedsConverting || force )
    {
        std::ofstream out( m_outputSettingsFile, std::ios::binary );
        if ( !out )
        {
            LOG_ERR( "Failed to open settings file '", m_outputSettingsFile, "'" );
            return CONVERT_ERROR;
        }
        serialize::Write( out, createInfo.name );
        serialize::Write( out, createInfo.flags );
        serialize::Write( out, createInfo.sampler );
    }

    if ( m_contentNeedsConverting || force )
    {
        Image image;
        ImageCreateInfo tmpCreateInfo = createInfo;
        tmpCreateInfo.flags &= ~IMAGE_FREE_CPU_COPY_ON_LOAD;
        tmpCreateInfo.flags &= ~IMAGE_CREATE_TEXTURE_ON_LOAD;
        if ( !image.Load( &tmpCreateInfo ) )
        {
            LOG_ERR( "Could not load image '", createInfo.name, "'" );
            return CONVERT_ERROR;
        }

        if ( Gfx::PixelFormatIsCompressed( createInfo.dstFormat ) )
        {
            Image compressedImage;
            if ( !CompressAndTranscodeImage( image, compressedImage, createInfo ) )
            {
                LOG_ERR( "Could not compress and transcode image" );
                return CONVERT_ERROR;
            }

            std::ofstream out( m_outputContentFile, std::ios::binary );
            if ( !compressedImage.Serialize( out ) )
            {
                LOG_ERR( "Could not save image '", createInfo.name, "' to fastfile" );
                out.close();
                std::filesystem::remove( m_outputContentFile );
                return CONVERT_ERROR;
            }
        }
        else
        {
            std::ofstream out( m_outputContentFile, std::ios::binary );
            if ( !image.Serialize( out ) )
            {
                LOG_ERR( "Could not save image '", createInfo.name, "' to fastfile" );
                out.close();
                std::filesystem::remove( m_outputContentFile );
                return CONVERT_ERROR;
            }
        }        
    }

    return CONVERT_SUCCESS;
}

std::string ImageConverter::GetName() const
{
    return createInfo.name;
}

static basist::transcoder_texture_format PGToBasisBCPixelFormat( Gfx::PixelFormat pgFormat, bool hasAlpha )
{
    PG_ASSERT( Gfx::PixelFormatIsCompressed( pgFormat ) );
    basist::transcoder_texture_format convert[] =
    {
        basist::transcoder_texture_format::cTFBC1_RGB,             // BC1_RGB_UNORM
        basist::transcoder_texture_format::cTFBC1_RGB,             // BC1_RGB_SRGB
        basist::transcoder_texture_format::cTFTotalTextureFormats, // BC1_RGBA_UNORM
        basist::transcoder_texture_format::cTFTotalTextureFormats, // BC1_RGBA_SRGB
        basist::transcoder_texture_format::cTFTotalTextureFormats, // BC2_UNORM
        basist::transcoder_texture_format::cTFTotalTextureFormats, // BC2_SRGB
        basist::transcoder_texture_format::cTFBC3_RGBA,            // BC3_UNORM
        basist::transcoder_texture_format::cTFBC3_RGBA,            // BC3_SRGB
        basist::transcoder_texture_format::cTFBC4_R,               // BC4_UNORM
        basist::transcoder_texture_format::cTFBC4_R,               // BC4_SNORM
        basist::transcoder_texture_format::cTFBC5_RG,              // BC5_UNORM (note: X = R and Y = Alpha)
        basist::transcoder_texture_format::cTFBC5_RG,              // BC5_SNORM (note: X = R and Y = Alpha)
        basist::transcoder_texture_format::cTFTotalTextureFormats, // BC6H_UFLOAT
        basist::transcoder_texture_format::cTFTotalTextureFormats, // BC6H_SFLOAT
        basist::transcoder_texture_format::cTFBC7_M6_OPAQUE_ONLY,  // BC7_UNORM cTFBC7_M6_OPAQUE_ONLY cDecodeFlagsTranscodeAlphaDataToOpaqueFormats
        basist::transcoder_texture_format::cTFBC7_M6_OPAQUE_ONLY,  // BC7_SRGB cTFBC7_M5_RGBA
    };

    auto basisFormat = convert[static_cast< int >( pgFormat ) - static_cast< int >( Gfx::PixelFormat::BC1_RGB_UNORM )];
    if ( hasAlpha && ( pgFormat == Gfx::PixelFormat::BC7_UNORM || pgFormat == Gfx::PixelFormat::BC7_SRGB ) )
    {
        basisFormat = basist::transcoder_texture_format::cTFBC7_M5_RGBA;
    }

    return basisFormat;
}

static bool TranscodeBasisImage( basisu::basis_compressor& compressor, Image& outputCompressedImage )
{
    void* data               = (void*) compressor.get_output_basis_file().data();
    uint32_t dataSizeInBytes = compressor.get_basis_file_size();

    basist::etc1_global_selector_codebook sel_codebook( basist::g_global_selector_cb_size, basist::g_global_selector_cb );
    basist::basisu_transcoder transcoder( &sel_codebook );
    if ( !transcoder.start_transcoding( data, dataSizeInBytes ) )
    {
        LOG_ERR( "Could not start transcoding" );
        return false;
    }
    basist::basisu_file_info fileInfo;
    transcoder.get_file_info( data, dataSizeInBytes, fileInfo );
    basist::transcoder_texture_format dstBasisTexFormat = PGToBasisBCPixelFormat( outputCompressedImage.GetPixelFormat(), fileInfo.m_has_alpha_slices );
    uint32_t dstTexFormatSize                           = Gfx::SizeOfPixelFromat( outputCompressedImage.GetPixelFormat() );

    basist::basis_texture_type basisTextureType = transcoder.get_texture_type( data, dataSizeInBytes );
    uint32_t imageCount = transcoder.get_total_images( data, dataSizeInBytes );
    basist::basisu_image_info firstImageInfo;
    transcoder.get_image_info( data, dataSizeInBytes, firstImageInfo, 0 );

    Gfx::ImageDescriptor imageDesc;
    if ( basisTextureType == basist::cBASISTexType2D )
    {
        if ( imageCount == 6 )
        {
            imageDesc.type = Gfx::ImageType::TYPE_CUBEMAP;
        }
        else
        {
            imageDesc.type = Gfx::ImageType::TYPE_2D;
        }
    }
    else if ( basisTextureType == basist::cBASISTexType2DArray )
    {
        imageDesc.type = Gfx::ImageType::TYPE_2D_ARRAY;
    }
    else if ( basisTextureType == basist::cBASISTexTypeCubemapArray )
    {
        if ( imageCount == 6 )
        {
            imageDesc.type = Gfx::ImageType::TYPE_CUBEMAP;
        }
        else
        {
            imageDesc.type = Gfx::ImageType::TYPE_CUBEMAP_ARRAY;
        }
    }
    else
    {
        PG_ASSERT( false, "Invalid texture type" );
    }
    imageDesc.arrayLayers = imageCount;
    imageDesc.width       = firstImageInfo.m_width;
    imageDesc.height      = firstImageInfo.m_height;
    imageDesc.mipLevels   = firstImageInfo.m_total_levels;
    imageDesc.format      = outputCompressedImage.GetPixelFormat();
    imageDesc.usage       = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    char* allTranscodedData = (char*) outputCompressedImage.GetPixels();
    char* currentSlice      = allTranscodedData;
    size_t currentSize = 0;
    for ( uint32_t imageIndex = 0; imageIndex < imageCount; ++imageIndex )
    {
        basist::basisu_image_info imageInfo;
        transcoder.get_image_info( data, dataSizeInBytes, imageInfo, imageIndex );

        for ( uint32_t mipLevel = 0; mipLevel < imageInfo.m_total_levels; ++mipLevel )
        {
            basist::basisu_image_level_info levelInfo;
            transcoder.get_image_level_info( data, dataSizeInBytes, levelInfo, imageIndex, mipLevel );
            uint32_t levelSize = levelInfo.m_total_blocks * dstTexFormatSize;

            bool didTranscode = transcoder.transcode_image_level( data, dataSizeInBytes, imageIndex, mipLevel, currentSlice, levelSize, dstBasisTexFormat );
            if( !didTranscode )
            {
                LOG_ERR( "Could not transcode image: ", imageIndex, ", mip: ", mipLevel );
                return false;
            }
            currentSize += levelSize;
            currentSlice += levelSize;
        }
    }

    return true;
}

static bool CompressImage( const Image& image, basisu::basis_compressor& compressor, const ImageCreateInfo& createInfo )
{
    basist::etc1_global_selector_codebook sel_codebook( basist::g_global_selector_cb_size, basist::g_global_selector_cb );

    uint32_t num_threads = 1;
    num_threads = std::thread::hardware_concurrency();
    if ( num_threads < 1 )
    {
	    num_threads = 1;
    }

    basisu::basis_compressor_params params = {};
    basisu::job_pool jpool( num_threads );
    params.m_pJob_pool                = &jpool;
    params.m_read_source_images       = false;
    params.m_write_output_basis_files = false;
    params.m_pSel_codebook            = &sel_codebook;
    if ( createInfo.compressionQuality == ImageCompressionQuality::LOW )
    {
        params.m_compression_level = 0;
        params.m_quality_level     = 50;
    }
    else if ( createInfo.compressionQuality == ImageCompressionQuality::MEDIUM )
    {
        params.m_compression_level = 1;
        params.m_quality_level     = 128;
    }
    else if ( createInfo.compressionQuality == ImageCompressionQuality::HIGH )
    {
        params.m_compression_level = 2;
        params.m_quality_level     = 255;
    }
    else if ( createInfo.compressionQuality == ImageCompressionQuality::MAX )
    {
        params.m_compression_level     = 5;
        params.m_quality_level         = 255;
        params.m_max_endpoint_clusters = basisu::basisu_frontend::cMaxEndpointClusters;
        params.m_max_selector_clusters = basisu::basisu_frontend::cMaxSelectorClusters;
    }
    params.m_perceptual        = false; // seems to look better for sRGB images too??
    params.m_y_flip            = false;
    params.m_mip_gen           = image.GetImageFlags() & IMAGE_GENERATE_MIPMAPS;
    params.m_source_images.resize( image.GetArrayLayers() );
    if ( createInfo.semantic == ImageSemantic::NORMAL )
    {
        params.m_seperate_rg_to_color_alpha = true;
        params.m_perceptual                 = false;
		params.m_mip_srgb                   = false;
		params.m_no_selector_rdo            = true;
		params.m_no_endpoint_rdo            = true;
    }
    PG_ASSERT( Gfx::SizeOfPixelFromat( image.GetPixelFormat() ) >= 3, "Not properly accounting for R or RG textures yet" );
    size_t faceSize = 4 * image.GetWidth() * image.GetHeight();
    for ( uint32_t i = 0; i < image.GetArrayLayers(); ++i )
    {
        params.m_source_images[i].resize( image.GetWidth(), image.GetHeight() );
        memcpy( params.m_source_images[i].get_ptr(), image.GetPixels() + i * faceSize, faceSize );
    }

    if ( !compressor.init( params ) )
    {
	    LOG_ERR( "basis_compressor::init() failed!" );
	    return false;
    }

    basisu::interval_timer tm;
	tm.start();
	basisu::basis_compressor::error_code ec = compressor.process();
	tm.stop();

	if ( ec == basisu::basis_compressor::cECSuccess )
	{
		LOG( "Compression succeeded in ", tm.get_elapsed_secs(), " secs" );
	}
	else
	{
		LOG_ERR( "Compression failed with error code: ", ec );
        return false;
	}

    return true;
}

static bool CompressAndTranscodeImage( const Image& uncompressedImage, Image& outputCompressedImage, const ImageCreateInfo& createInfo  )
{
    Gfx::ImageDescriptor compressedDesc = uncompressedImage.GetDescriptor();
    if ( createInfo.flags & IMAGE_GENERATE_MIPMAPS )
    {
        compressedDesc.mipLevels = static_cast< uint32_t >( 1 + std::floor( std::log2( std::max( compressedDesc.width, compressedDesc.height ) ) ) );
    }
    compressedDesc.format = createInfo.dstFormat;
    outputCompressedImage = Image( compressedDesc );

    basisu::basis_compressor compressor;
    if ( !CompressImage( uncompressedImage, compressor, createInfo ) )
    {
        LOG_ERR( "Could not compress image to basis format" );
        return false;
    }
    if ( !TranscodeBasisImage( compressor, outputCompressedImage ) )
    {
        LOG_ERR( "Could not transcode basis image" );
        return false;
    }

    return true;
}