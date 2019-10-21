#include "resource/converters/script_converter.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#include "utils/timestamp.hpp"
#include <filesystem>
#include <fstream>

using namespace Progression;

static std::string GetContentFastFileName( const ScriptCreateInfo& createInfo )
{
    namespace fs = std::filesystem;

    std::string fname = fs::path( createInfo.filename ).filename().string();

    size_t hash = std::hash< std::string >{}( createInfo.filename );
    return PG_RESOURCE_DIR "cache/scripts/" + fname + "_" + std::to_string( hash ) + ".ffi";
}

static std::string GetSettingsFastFileName( const ScriptCreateInfo& createInfo )
{
    return PG_RESOURCE_DIR "cache/scripts/settings_" + createInfo.name + ".ffi";
}

AssetStatus ScriptConverter::CheckDependencies()
{
    m_outputContentFile  = GetContentFastFileName( createInfo );
    m_outputSettingsFile = GetSettingsFastFileName( createInfo );

    IF_VERBOSE_MODE( LOG( "\tScript with name '", createInfo.name, "' and filename '", createInfo.filename, "' outputs:" ) );
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

    if ( !std::filesystem::exists( m_outputContentFile ) )
    {
        LOG( "OUT OF DATE: FFI File for script '", createInfo.name, "' does not exist, convert required" );
        m_contentNeedsConverting = true;
        return ASSET_OUT_OF_DATE;
    }

    Timestamp outTimestamp( m_outputContentFile );
    Timestamp inputTimestamp( createInfo.filename );
    IF_VERBOSE_MODE( LOG( "Script timestamp: ", inputTimestamp, ", FFI timestamp: ", outTimestamp ) );
    m_contentNeedsConverting = outTimestamp <= inputTimestamp;

    if ( m_contentNeedsConverting )
    {
        LOG( "OUT OF DATE: Script file'", createInfo.name, "' has newer timestamp than saved FFI" );
        m_contentNeedsConverting = true;
    }

    if ( m_settingsNeedsConverting || m_contentNeedsConverting )
    {
        m_status = ASSET_OUT_OF_DATE;
    }
    else
    {
        if ( force )
        {
            LOG( "UP TO DATE: Script with name '", createInfo.name, "', but --force used, so converting anyways\n" );
            m_status = ASSET_OUT_OF_DATE;
        }
        else
        {
            m_status = ASSET_UP_TO_DATE;
            LOG( "UP TO DATE: Script with name '", createInfo.name, "'" );
        }
    }

    return m_status;
}

ConverterStatus ScriptConverter::Convert()
{
    if ( m_status == ASSET_UP_TO_DATE )
    {
        return CONVERT_SUCCESS;
    }

    if ( m_settingsNeedsConverting || force )
    {
        std::ofstream out( m_outputSettingsFile, std::ios::binary );
        serialize::Write( out, createInfo.name );
    }

    if ( m_contentNeedsConverting || force )
    {
        Script s;
        if ( !s.Load( &createInfo ) )
        {
            return CONVERT_ERROR;
        }

        std::ofstream out( m_outputContentFile, std::ios::binary );
        serialize::Write( out, s.scriptText );
    }

    return CONVERT_SUCCESS;
}

std::string ScriptConverter::GetName() const
{
    return createInfo.name;
}
