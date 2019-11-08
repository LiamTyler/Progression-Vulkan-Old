#include "resource/converters/shader_converter.hpp"
#include "resource/shader.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#include "utils/timestamp.hpp"
#include <filesystem>
#include <fstream>
#include <array>
#include <unordered_set>

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

static bool PreprocessShader( const std::string& filename, std::string& text, std::unordered_set< std::string >& includedFiles )
{
    std::ifstream in( filename );
    if ( !in )
    {
        LOG_ERR( "Could not open file '", filename, "'" );
        return false;
    }

    static std::array< std::string, 3 > includeDirs =
    {
        PG_ROOT_DIR "progression/",
        PG_RESOURCE_DIR "shaders/"
        PG_RESOURCE_DIR,
        PG_ROOT_DIR,
    };

    std::string line;
    while ( std::getline( in, line ) )
    {
        if ( line == "#pragma once" )
        {
            continue;
        }
        std::string firstWord;
        auto whitespaceBegin = line.find_first_of( " \t" );
        if ( whitespaceBegin != std::string::npos )
        {
            firstWord = line.substr( 0, whitespaceBegin );
        }

        if ( firstWord == "#include" )
        {
            text += "// " + line + "\n";
            auto firstParen  = line.find( '\"' );
            auto secondParen = line.find( '\"', firstParen + 1 );
            if ( firstParen == std::string::npos || secondParen == std::string::npos )
            {
                LOG_ERR( "line '", line, "' is not a valid #include \"some_filename\" line" );
                return false;
            }
            std::string includeFilename = line.substr( firstParen + 1, secondParen - firstParen - 1 );

            bool alreadyIncluded = false;
            for ( const auto& dir : includeDirs )
            {
                if ( includedFiles.find( dir + includeFilename ) != includedFiles.end() )
                {
                    alreadyIncluded = true;
                    break;
                }
            }
            if ( alreadyIncluded )
            {
                continue;
            }
            std::string fullIncludePath;
            for ( const auto& dir : includeDirs )
            {
                std::string fullpath = dir + includeFilename;
                if ( std::filesystem::exists( fullpath ) )
                {
                    if ( !PreprocessShader( fullpath, text, includedFiles ) )
                    {
                        LOG_ERR( "Could not #include file '", fullpath, "'" );
                        return false;
                    }
                    fullIncludePath = fullpath;
                    break;
                }
            }
            if ( fullIncludePath.empty() )
            {
                LOG_ERR( "From file '", filename, "' could not #include \"", includeFilename, "\"" );
                return false;
            }
            includedFiles.insert( fullIncludePath );
        }
        else
        {
            text += line + "\n";
        }
    }

    return true;
}

AssetStatus ShaderConverter::CheckDependencies()
{
    m_outputContentFile  = GetContentFastFileName( createInfo );
    m_outputSettingsFile = GetSettingsFastFileName( createInfo );

    IF_VERBOSE_MODE( LOG( "\nShader with name '", createInfo.name, "' and filename '", createInfo.filename, "' outputs:" ) );
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
        LOG( "OUT OF DATE: FFI File for shader '", createInfo.name, "' does not exist, convert required" );
        m_contentNeedsConverting = true;
        return ASSET_OUT_OF_DATE;
    }

    Timestamp outTimestamp( m_outputContentFile );
    Timestamp inputTimestamp( createInfo.filename );
    IF_VERBOSE_MODE( LOG( "Shader timestamp: ", inputTimestamp, ", FFI timestamp: ", outTimestamp ) );
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
        if ( force )
        {
            LOG( "UP TO DATE: Shader with name '", createInfo.name, "', but --force used, so converting anyways\n" );
            m_status = ASSET_OUT_OF_DATE;
        }
        else
        {
            m_status = ASSET_UP_TO_DATE;
            LOG( "UP TO DATE: Shader with name '", createInfo.name, "'" );
        }
    }

    return m_status;
}

ConverterStatus ShaderConverter::Convert()
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
        std::string preprocessedShaderText;
        std::unordered_set< std::string > includedFiles;
        if ( !PreprocessShader( createInfo.filename, preprocessedShaderText, includedFiles ) )
        {
            LOG_ERR( "Could not preprocess the shader '", createInfo.filename, "'" );
            return CONVERT_ERROR;
        }

        std::string originalExtension = std::filesystem::path( createInfo.filename ).extension().string();
        std::string preprocessFilename = m_outputContentFile.substr( 0, m_outputContentFile.find_last_of( '.' ) ) + "_preprocess" + originalExtension;
        std::ofstream preprocOut( preprocessFilename );
        if ( !preprocOut )
        {
            LOG_ERR( "Could not open file preprocess shader file" );
            return CONVERT_ERROR;
        }
        preprocOut << preprocessedShaderText;
        preprocOut.close();

        std::string command = "glslc \"" + preprocessFilename + "\" -o \"" + m_outputContentFile + "\"";
        LOG( "Compiling shader '", createInfo.name );
        LOG( command );
        int ret = system( command.c_str() );
        if ( ret != 0 )
        {
            LOG_ERR( "Error while compiling shader: ", createInfo.name );
            return CONVERT_ERROR;
        }

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
        for ( const auto& set : reflectInfo.descriptorSetLayouts )
        {
            serialize::Write( out, set.setNumber );
            serialize::Write( out, set.createInfo );
            serialize::Write( out, set.bindings );
        }
        serialize::Write( out, reflectInfo.pushConstants );
        serialize::Write( out, buffer );
    }

    return CONVERT_SUCCESS;
}

std::string ShaderConverter::GetName() const
{
    return createInfo.name;
}
