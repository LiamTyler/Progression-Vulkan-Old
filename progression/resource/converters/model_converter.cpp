#include "resource/converters/model_converter.hpp"
#include "core/assert.hpp"
#include "resource/model.hpp"
#include "resource/resource_version_numbers.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#include "utils/timestamp.hpp"
#include <filesystem>
#include <fstream>

using namespace Progression;

namespace fs = std::filesystem;

static std::string GetContentFastFileName( struct ModelCreateInfo& createInfo )
{
    PG_ASSERT( !createInfo.filename.empty() );
    fs::path filePath = fs::absolute( createInfo.filename );

    std::string hash     = std::to_string( std::hash< std::string >{}( filePath.string() ) );
    std::string baseName = filePath.filename().string();
    std::string version  = std::to_string( PG_RESOURCE_MODEL_VERSION );

    return PG_RESOURCE_DIR "cache/models/" + baseName + "_" + version + "_" + hash + ".ffi";
}

static std::string GetSettingsFastFileName( const ModelCreateInfo& createInfo )
{
    PG_ASSERT( !createInfo.filename.empty() );

    int optimize    = createInfo.optimize;
    int freeCpuCopy = createInfo.freeCpuCopy;

    return PG_RESOURCE_DIR "cache/models/settings_" + createInfo.name +
        std::to_string( optimize ) + std::to_string( freeCpuCopy ) + ".ffi";
}

AssetStatus ModelConverter::CheckDependencies()
{
    PG_ASSERT( !createInfo.filename.empty() );

    m_outputContentFile  = GetContentFastFileName( createInfo );
    m_outputSettingsFile = GetSettingsFastFileName( createInfo );

    IF_VERBOSE_MODE( LOG( "\nModel with name '", createInfo.name, "' and filename '", createInfo.filename, "' outputs:" ) );
    IF_VERBOSE_MODE( LOG( "\tContentFile: ", m_outputContentFile ) );
    IF_VERBOSE_MODE( LOG( "\tSettingsFile: ", m_outputSettingsFile ) );

    if ( !std::filesystem::exists( m_outputSettingsFile ) )
    {
        LOG( "OUT OF DATE: FFI File for settings file '", m_outputSettingsFile, "' does not exist, needs to be generated" );
        m_settingsNeedsConverting = true;
    }
    else
    {
        m_settingsNeedsConverting = false;
    }

    auto path = fs::path( createInfo.filename );
    std::string matFile = path.parent_path().string() + "/" + path.stem().string() + ".mtl";
    Timestamp outTimestamp( m_outputContentFile );
    Timestamp mtlTimestamp( matFile );
    Timestamp objFileTime( createInfo.filename );
    IF_VERBOSE_MODE( LOG( "Input model timestamp: ", objFileTime, " MTL timestamp: ", mtlTimestamp,
                          ", FFI timestamp: ", outTimestamp ) );

    m_contentNeedsConverting = outTimestamp <= objFileTime || outTimestamp <= mtlTimestamp;
    m_status = m_contentNeedsConverting ||  m_settingsNeedsConverting ? ASSET_OUT_OF_DATE : ASSET_UP_TO_DATE;

    if ( outTimestamp <= mtlTimestamp )
    {
        LOG( "OUT OF DATE: MTL file '", matFile, "' for model '", createInfo.filename, "' has newer timestamp than saved FFI" );
    }

    if ( outTimestamp <= objFileTime )
    {
        LOG( "OUT OF DATE: Model file '", createInfo.filename, "' has newer timestamp than saved FFI" );
    }

    if ( m_status == ASSET_UP_TO_DATE )
    {
        if ( force )
        {
            LOG( "UP TO DATE: Model file '", createInfo.name , "' but --force used, so converting anyways\n" );
            m_status = ASSET_OUT_OF_DATE;
        }
        else
        {
            LOG( "UP TO DATE: Model file '", createInfo.name , "'" );
        }
    }

    return m_status;
}

ConverterStatus ModelConverter::Convert()
{
    if ( m_status == ASSET_UP_TO_DATE )
    {
        return CONVERT_SUCCESS;
    }

    if ( m_settingsNeedsConverting || force )
    {
        std::ofstream out( m_outputSettingsFile, std::ios::binary );
        serialize::Write( out, createInfo.name );
        serialize::Write( out, createInfo.freeCpuCopy );
        serialize::Write( out, createInfo.createGpuCopy );
    }

    if ( m_contentNeedsConverting || force )
    {
        Model model;
        createInfo.freeCpuCopy   = false;
        createInfo.createGpuCopy = false;
        if ( !model.Load( &createInfo ) )
        {
            LOG_ERR( "Could not load the model '", createInfo.filename, "'" );
            return CONVERT_ERROR;
        }

        std::ofstream out( m_outputContentFile, std::ios::binary );

        if ( !model.Serialize( out ) )
        {
            LOG_ERR( "Could not serialize the model '", createInfo.filename, "'" );
            return CONVERT_ERROR;
        }
    }

    return CONVERT_SUCCESS;
}

std::string ModelConverter::GetName() const
{
    return createInfo.name;
}
