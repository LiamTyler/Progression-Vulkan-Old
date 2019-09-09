#include "resource/converters/image_converter.hpp"
#include "core/assert.hpp"
#include "resource/image.hpp"
#include "resource/resource_manager.hpp"
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
    PG_ASSERT( !createInfo.filename.empty() );
    fs::path filePath = fs::absolute( createInfo.filename );

    size_t hash          = std::hash< std::string >{}( filePath.string() );
    std::string baseName = filePath.filename().string();

    return PG_RESOURCE_DIR "cache/images/" + baseName + std::to_string( hash ) + ".ffi";
}

static std::string GetSettingsFastFileName( const ImageCreateInfo& createInfo )
{
    PG_ASSERT( !createInfo.filename.empty() );

    return PG_RESOURCE_DIR "cache/images/settings_" + createInfo.name +
           std::to_string( static_cast< int >( createInfo.flags ) ) + ".ffi";
}

AssetStatus ImageConverter::CheckDependencies()
{
    PG_ASSERT( !createInfo.filename.empty() );

    // Add an empty image to the manager to notify other resources that an image with this
    // name has already been loaded
    auto placeHolderImg  = std::make_shared< Image >();
    placeHolderImg->name = createInfo.name;
    ResourceManager::Add< Image >( placeHolderImg );

    m_outputContentFile  = GetContentFastFileName( createInfo );
    m_outputSettingsFile = GetSettingsFastFileName( createInfo );

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
        LOG( "OUT OF DATE: FFI File for image '", createInfo.filename, "' does not exist, convert required" );
        m_contentNeedsConverting = true;
    }
    else
    {
        if ( Timestamp( m_outputContentFile ) < Timestamp( createInfo.filename ) )
        {
            LOG( "OUT OF DATE: Image '", createInfo.filename, "' has newer timestamp than saved FFI" );
            m_contentNeedsConverting = true;
        }
        else
        {
            m_contentNeedsConverting = false;
        }
    }

    if ( m_settingsNeedsConverting || m_contentNeedsConverting )
    {
        m_status = ASSET_OUT_OF_DATE;
    }
    else
    {
        m_status = ASSET_UP_TO_DATE;
        LOG( "UP TO DATE: Image with name '", createInfo.name, "' and filename '", createInfo.filename, "'" );
    }

    return m_status;
}

ConverterStatus ImageConverter::Convert()
{
    if ( m_status == ASSET_UP_TO_DATE )
    {
        return CONVERT_SUCCESS;
    }

    if ( m_settingsNeedsConverting )
    {
        std::ofstream out( m_outputSettingsFile, std::ios::binary );
        serialize::Write( out, createInfo.name );
        serialize::Write( out, createInfo.flags );
    }

    if ( m_contentNeedsConverting )
    {
        Image image;
        if ( !image.Load( &createInfo ) )
        {
            LOG_ERR( "Could not load image '", createInfo.filename, "'" );
            return CONVERT_ERROR;
        }

        std::ofstream out( m_outputContentFile, std::ios::binary );
        if ( !image.Serialize( out ) )
        {
            LOG_ERR( "Could not save image '", createInfo.filename, "' to fastfile" );
            std::filesystem::remove( m_outputContentFile );
            return CONVERT_ERROR;
        }
    }

    return CONVERT_SUCCESS;
}
