#include "resource/converters/image_converter.hpp"
#include "core/assert.hpp"
#include "graphics/render_system.hpp"
#include "resource/image.hpp"
#include "resource/resource_manager.hpp"
#include "resource/resource_version_numbers.hpp"
#include "utils/fileIO.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#include "utils/timestamp.hpp"
#include <filesystem>
#include <fstream>

using namespace Progression;
namespace fs = std::filesystem;

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
    std::string flip     = createInfo.flags & IMAGE_FLIP_VERTICALLY ? "1" : "0";
    std::string version  = std::to_string( PG_RESOURCE_IMAGE_VERSION );

    return PG_RESOURCE_DIR "cache/images/" + baseName + "_" + flip + "_" + version + "_" + hash + ".ffi";
}

static std::string GetSettingsFastFileName( const ImageCreateInfo& createInfo )
{
    return PG_RESOURCE_DIR "cache/images/settings_" + createInfo.name + "_" + createInfo.sampler + "_" +
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
            LOG( "UP TO DATE: Image with name '", createInfo.name, "', but --force used, so converting anyways\n" );
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
        serialize::Write( out, createInfo.name );
        serialize::Write( out, createInfo.flags );
        serialize::Write( out, createInfo.sampler );
    }

    if ( m_contentNeedsConverting || force )
    {
        Image image;
        createInfo.flags &= ~IMAGE_FREE_CPU_COPY_ON_LOAD;
        createInfo.flags &= ~IMAGE_CREATE_TEXTURE_ON_LOAD;
        createInfo.flags &= ~IMAGE_GENERATE_MIPMAPS;
        if ( !image.Load( &createInfo ) )
        {
            LOG_ERR( "Could not load image '", createInfo.name, "'" );
            return CONVERT_ERROR;
        }

        std::ofstream out( m_outputContentFile, std::ios::binary );
        if ( !image.Serialize( out ) )
        {
            LOG_ERR( "Could not save image '", createInfo.name, "' to fastfile" );
            out.close();
            std::filesystem::remove( m_outputContentFile );
            return CONVERT_ERROR;
        }
    }

    return CONVERT_SUCCESS;
}

std::string ImageConverter::GetName() const
{
    return createInfo.name;
}