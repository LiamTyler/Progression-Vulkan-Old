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

    std::string fname = fs::path( createInfo.filename ).filename().string();

    size_t hash = std::hash< std::string >{}( createInfo.filename );
    return PG_RESOURCE_DIR "cache/shaders/" + fname + "_" + std::to_string( hash ) + ".ffi";
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
    Timestamp inputTimestamp( createInfo.filename );
    m_contentNeedsConverting = outTimestamp <= inputTimestamp;

    if ( m_contentNeedsConverting )
    {
        LOG( "OUT OF DATE: Shader file'", createInfo.name, "' has newer timestamp than saved FFI" );
        m_contentNeedsConverting = true;
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
//#if USING( LINUX_PROGRAM )
        // std::string outputFile = createInfo.filename + ".spv";
        std::string command = "glslc " + createInfo.filename + " -o " + m_outputContentFile;
        LOG( "Compiling shader '", createInfo.name );
        LOG( command );
        int ret = system( command.c_str() );
        if ( ret != 0 )
        {
            LOG_ERR( "Error while compiling shader: ", createInfo.name );
            return CONVERT_ERROR;
        }
//#endif // #if USING( LINUX_PROGRAM )

        std::ifstream file( m_outputContentFile, std::ios::ate | std::ios::binary );
        size_t fileSize = static_cast< size_t >( file.tellg() );
        std::vector< char > buffer( fileSize );
        file.seekg( 0 );
        file.read( buffer.data(), fileSize );
        file.close();

        ShaderReflectInfo reflectInfo = Shader::Reflect( (const uint32_t* ) buffer.data(), fileSize );
        std::ofstream out( m_outputContentFile, std::ios::binary );
        serialize::Write( out, reflectInfo.entryPoint );
        serialize::Write( out, reflectInfo.stage );
        serialize::Write( out, reflectInfo.inputLocations.size() );
        for ( const auto& [varName, varLoc] : reflectInfo.inputLocations )
        {
            serialize::Write( out, varName );
            serialize::Write( out, varLoc );
        }
        serialize::Write( out, reflectInfo.outputLocations.size() );
        for ( const auto& [varName, varLoc] : reflectInfo.outputLocations )
        {
            serialize::Write( out, varName );
            serialize::Write( out, varLoc );
        }
        serialize::Write( out, reflectInfo.descriptorSetLayouts.size() );
        LOG( "num descs: ", reflectInfo.descriptorSetLayouts.size() );
        for ( const auto& set : reflectInfo.descriptorSetLayouts )
        {
            serialize::Write( out, set.setNumber );
            serialize::Write( out, set.createInfo );
            serialize::Write( out, set.bindings );
        }
        serialize::Write( out, buffer );
    }

    return CONVERT_SUCCESS;
}

std::string ShaderConverter::GetName() const
{
    return createInfo.name;
}
