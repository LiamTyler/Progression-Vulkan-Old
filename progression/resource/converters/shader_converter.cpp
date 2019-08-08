#include "resource/converters/shader_converter.hpp"
#include "resource/shader.hpp"
#include "utils/serialize.hpp"
#include "utils/timestamp.hpp"
#include <filesystem>
#include <fstream>

using namespace Progression;

static std::string GetShaderFastFileName( struct ShaderCreateInfo& createInfo )
{
    namespace fs = std::filesystem;

    std::string vertPath = createInfo.vertex.empty() ? "" : fs::absolute( createInfo.vertex );
    std::string geomPath = createInfo.geometry.empty() ? "" : fs::absolute( createInfo.geometry );
    std::string fragPath = createInfo.fragment.empty() ? "" : fs::absolute( createInfo.fragment );
    std::string compPath = createInfo.compute.empty() ? "" : fs::absolute( createInfo.compute );

    size_t hash = std::hash< std::string >{}( vertPath + geomPath + fragPath + compPath );
    vertPath    = fs::path( createInfo.vertex ).filename();
    geomPath    = fs::path( createInfo.geometry ).filename();
    fragPath    = fs::path( createInfo.fragment ).filename();
    compPath    = fs::path( createInfo.compute ).filename();
    return PG_RESOURCE_DIR "cache/shaders/" + vertPath + "_" + geomPath + "_" + fragPath + "_" +
           compPath + "_" + std::to_string( hash ) + ".ffi";
}

AssetStatus ShaderConverter::CheckDependencies()
{
    if ( outputFile.empty() )
    {
        outputFile = GetShaderFastFileName( createInfo );
    }
    if ( !std::filesystem::exists( outputFile ) )
    {
        LOG( "OUT OF DATE: FFI File for Shader '", createInfo.name, "' does not exist, convert required" );
        return ASSET_OUT_OF_DATE;
    }

    Timestamp outTimestamp( outputFile );

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

    m_status = outTimestamp <= newestFileTime ? ASSET_OUT_OF_DATE : ASSET_UP_TO_DATE;

    if ( m_status == ASSET_OUT_OF_DATE )
    {
        LOG( "OUT OF DATE: Shader file'", createInfo.name, "' has newer timestamp than saved FFI" );
    }
    else
    {
        LOG( "UP TO DATE: Shader '", createInfo.name, "'" );
    }

    return m_status;
}

ConverterStatus ShaderConverter::Convert()
{
    if ( !force && m_status == ASSET_UP_TO_DATE )
    {
        return CONVERT_SUCCESS;
    }

    Progression::Shader shader;

    if ( !shader.Load( &createInfo ) )
    {
        LOG_ERR( "Could not load the shader" );
        return CONVERT_ERROR;
    }

    std::ofstream out( outputFile, std::ios::binary );

    if ( !shader.Serialize( out ) )
    {
        LOG_ERR( "Could not save shader to fastfile" );
        return CONVERT_ERROR;
    }

    out.close();

    return CONVERT_SUCCESS;
}
