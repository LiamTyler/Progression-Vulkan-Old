#include "resource/converters/shader_converter.hpp"
#include "resource/shader.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#include "utils/timestamp.hpp"
#include <filesystem>
#include <fstream>

using namespace Progression;

static std::string GetContentFastFileName( struct ShaderCreateInfo& createInfo )
{
    namespace fs = std::filesystem;

    std::string vertPath = createInfo.vertex.empty()   ? "" : fs::absolute( createInfo.vertex ).string();
    std::string geomPath = createInfo.geometry.empty() ? "" : fs::absolute( createInfo.geometry ).string();
    std::string fragPath = createInfo.fragment.empty() ? "" : fs::absolute( createInfo.fragment ).string();
    std::string compPath = createInfo.compute.empty()  ? "" : fs::absolute( createInfo.compute ).string();

    size_t hash = std::hash< std::string >{}( vertPath + geomPath + fragPath + compPath );
    vertPath    = fs::path( createInfo.vertex ).filename().string();
    geomPath    = fs::path( createInfo.geometry ).filename().string();
    fragPath    = fs::path( createInfo.fragment ).filename().string();
    compPath    = fs::path( createInfo.compute ).filename().string();
    return PG_RESOURCE_DIR "cache/shaders/" + vertPath + "_" + geomPath + "_" + fragPath + "_" +
           compPath + "_" + std::to_string( hash ) + ".ffi";
}

static std::string GetSettingsFastFileName( const ShaderCreateInfo& createInfo )
{
    return PG_RESOURCE_DIR "cache/shaders/settings_" + createInfo.name + ".ffi";
}

AssetStatus ShaderConverter::CheckDependencies()
{
    m_outputContentFile  = GetContentFastFileName( createInfo );
    m_outputSettingsFile = GetSettingsFastFileName( createInfo );

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
        LOG( "OUT OF DATE: FFI File for shader '", createInfo.name, "' does not exist, convert required" );
        m_contentNeedsConverting = true;
        return ASSET_OUT_OF_DATE;
    }

    Timestamp outTimestamp( m_outputContentFile );

    Timestamp newestFileTime;
    if ( !createInfo.vertex.empty() )
    {
        Timestamp t( createInfo.vertex );
        newestFileTime = std::max( t, newestFileTime );
    }
    if ( !createInfo.geometry.empty() )
    {
        Timestamp t( createInfo.geometry );
        newestFileTime = std::max( t, newestFileTime );
    }
    if ( !createInfo.fragment.empty() )
    {
        Timestamp t( createInfo.fragment );
        newestFileTime = std::max( t, newestFileTime );
    }
    if ( !createInfo.compute.empty() )
    {
        Timestamp t( createInfo.compute );
        newestFileTime = std::max( t, newestFileTime );
    }

    m_contentNeedsConverting = outTimestamp <= newestFileTime;
    if ( m_contentNeedsConverting )
    {
        LOG( "OUT OF DATE: Shader file'", createInfo.name, "' has newer timestamp than saved FFI" );
        m_contentNeedsConverting = true;
    }
    else
    {
        m_contentNeedsConverting = false;
    }

    if ( m_settingsNeedsConverting || m_contentNeedsConverting )
    {
        m_status = ASSET_OUT_OF_DATE;
    }
    else
    {
        m_status = ASSET_UP_TO_DATE;
        LOG( "UP TO DATE: Shader with name '", createInfo.name, "'" );
    }

    return m_status;
}

ConverterStatus ShaderConverter::Convert()
{
    if ( m_status == ASSET_UP_TO_DATE )
    {
        return CONVERT_SUCCESS;
    }

    if ( m_settingsNeedsConverting )
    {
        std::ofstream out( m_outputSettingsFile, std::ios::binary );
        serialize::Write( out, createInfo.name );
    }

    if ( m_contentNeedsConverting )
    {
        Progression::Shader shader;

        if ( !shader.Load( &createInfo ) )
        {
            LOG_ERR( "Could not load the shader '", createInfo.name, "'" );
            return CONVERT_ERROR;
        }

        std::ofstream out( m_outputContentFile, std::ios::binary );

        if ( !shader.Serialize( out ) )
        {
            LOG_ERR( "Could not save shader to fastfile" );
            return CONVERT_ERROR;
        }
    }

    return CONVERT_SUCCESS;
}
