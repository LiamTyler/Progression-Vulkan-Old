#include "resource/converters/fastfile_converter.hpp"
#include "core/configuration.hpp"
#include "graphics/graphics_api.hpp"
#include "resource/shader.hpp"
#include "resource/texture.hpp"
#include "utils/fileIO.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#include "utils/timestamp.hpp"
#include <filesystem>
#include <fstream>

using namespace Progression;

bool ParseShaderCreateInfoFromFile( std::istream& in, ShaderCreateInfo& info )
{
    fileIO::parseLineKeyVal( in, "name", info.name );
    fileIO::parseLineKeyValOptional( in, "vertex", info.vertex );
    fileIO::parseLineKeyValOptional( in, "geometry", info.geometry );
    fileIO::parseLineKeyValOptional( in, "fragment", info.fragment );
    fileIO::parseLineKeyValOptional( in, "compute", info.compute );
    if ( !info.vertex.empty() )
    {
        info.vertex = PG_RESOURCE_DIR + info.vertex;
    }
    if ( !info.geometry.empty() )
    {
        info.geometry = PG_RESOURCE_DIR + info.geometry;
    }
    if ( !info.fragment.empty() )
    {
        info.fragment = PG_RESOURCE_DIR + info.fragment;
    }
    if ( !info.compute.empty() )
    {
        info.compute = PG_RESOURCE_DIR + info.compute;
    }

    return true;
}

static std::unordered_map< std::string, Gfx::PixelFormat > internalFormatMap = {
    { "R8_Uint", Gfx::PixelFormat::R8_Uint },
    { "R16_Float", Gfx::PixelFormat::R16_Float },
    { "R32_Float", Gfx::PixelFormat::R32_Float },
    { "R8_G8_Uint", Gfx::PixelFormat::R8_G8_Uint },
    { "R16_G16_Float", Gfx::PixelFormat::R16_G16_Float },
    { "R32_G32_Float", Gfx::PixelFormat::R32_G32_Float },
    { "R8_G8_B8_Uint", Gfx::PixelFormat::R8_G8_B8_Uint },
    { "R16_G16_B16_Float", Gfx::PixelFormat::R16_G16_B16_Float },
    { "R32_G32_B32_Float", Gfx::PixelFormat::R32_G32_B32_Float },
    { "R8_G8_B8_A8_Uint", Gfx::PixelFormat::R8_G8_B8_A8_Uint },
    { "R16_G16_B16_A16_Float", Gfx::PixelFormat::R16_G16_B16_A16_Float },
    { "R32_G32_B32_A32_Float", Gfx::PixelFormat::R32_G32_B32_A32_Float },
    { "R8_G8_B8_Uint_sRGB", Gfx::PixelFormat::R8_G8_B8_Uint_sRGB },
    { "R8_G8_B8_A8_Uint_sRGB", Gfx::PixelFormat::R8_G8_B8_A8_Uint_sRGB },
    { "R11_G11_B10_Float", Gfx::PixelFormat::R11_G11_B10_Float },
    { "DEPTH32_Float", Gfx::PixelFormat::DEPTH32_Float },
};

static std::unordered_map< std::string, Gfx::FilterMode > minFilterMap = {
    { "nearest", Gfx::FilterMode::NEAREST },
    { "linear", Gfx::FilterMode::LINEAR },
    { "nearest_mipmap_nearest", Gfx::FilterMode::NEAREST_MIPMAP_NEAREST },
    { "linear_mipmap_nearest", Gfx::FilterMode::LINEAR_MIPMAP_NEAREST },
    { "nearest_mipmap_linear", Gfx::FilterMode::NEAREST_MIPMAP_LINEAR },
    { "linear_mipmap_linear", Gfx::FilterMode::LINEAR_MIPMAP_LINEAR },
};

static std::unordered_map< std::string, Gfx::FilterMode > magFilterMap = {
    { "nearest", Gfx::FilterMode::NEAREST },
    { "linear", Gfx::FilterMode::LINEAR },
};

static std::unordered_map< std::string, Gfx::WrapMode > wrapMap = {
    { "clamp_to_edge", Gfx::WrapMode::CLAMP_TO_EDGE },
    { "clamp_to_border", Gfx::WrapMode::CLAMP_TO_BORDER },
    { "mirror_repeat", Gfx::WrapMode::MIRRORED_REPEAT },
    { "repeat", Gfx::WrapMode::REPEAT },
};

bool ParseTextureCreateInfoFromFile( std::istream& in, TextureCreateInfo& info )
{
    info.texDesc.type = Gfx::TextureType::TEXTURE2D;

    fileIO::parseLineKeyVal( in, "name", info.name );
    fileIO::parseLineKeyVal( in, "filename", info.filename );
    info.filename          = PG_RESOURCE_DIR + info.filename;
    info.texDesc.mipmapped = fileIO::parseLineKeyBool( in, "mipmapped" );
    if ( !fileIO::parseLineKeyMap( in, "internalFormat", internalFormatMap, info.texDesc.format ) )
    {
        LOG_ERR( "Invalid texture2D 'internalFormat'" );
        return false;
    }
    if ( !fileIO::parseLineKeyMap( in, "minFilter", minFilterMap, info.samplerDesc.minFilter ) )
    {
        LOG_ERR( "Invalid texture2D minFilter" );
        return false;
    }
    if ( !info.texDesc.mipmapped && ( info.samplerDesc.minFilter != Gfx::FilterMode::NEAREST &&
                                      info.samplerDesc.minFilter != Gfx::FilterMode::LINEAR ) )
    {
        LOG_ERR( "Trying to use a mipmap min filter when there is no mip map on texture: ",
                 info.filename );
        return false;
    }
    if ( !fileIO::parseLineKeyMap( in, "magFilter", magFilterMap, info.samplerDesc.magFilter ) )
    {
        LOG_ERR( "Invalid texture2D magFilter" );
        return false;
    }
    if ( !fileIO::parseLineKeyMap( in, "wrapModeS", wrapMap, info.samplerDesc.wrapModeS ) )
    {
        LOG_ERR( "Invalid texture2D wrapModeS" );
        return false;
    }
    if ( !fileIO::parseLineKeyMap( in, "wrapModeT", wrapMap, info.samplerDesc.wrapModeT ) )
    {
        LOG_ERR( "Invalid texture2D wrapModeT" );
        return false;
    }
    if ( !fileIO::parseLineKeyMap( in, "wrapModeR", wrapMap, info.samplerDesc.wrapModeR ) )
    {
        LOG_ERR( "Invalid texture2D wrapModeR" );
        return false;
    }
    fileIO::parseLineKeyVal( in, "borderColor", info.samplerDesc.borderColor );
    fileIO::parseLineKeyVal( in, "maxAnisotropy", info.samplerDesc.maxAnisotropy );

    return true;
}

bool ParseMaterialFileFromFile( std::istream& in, std::string& mtlFileName )
{
    fileIO::parseLineKeyVal( in, "filename", mtlFileName );
    mtlFileName = PG_RESOURCE_DIR + mtlFileName;
    return true;
}

AssetStatus FastfileConverter::CheckDependencies()
{
    std::ifstream in( inputFile );
    if ( !in )
    {
        LOG_ERR( "Could not open the input resource file: '", inputFile, "'" );
        return ASSET_CHECKING_ERROR;
    }

    m_status = ASSET_UP_TO_DATE;

    if ( outputFile.empty() )
    {
        outputFile = PG_RESOURCE_DIR "fastfiles/" +
                     std::filesystem::path( inputFile ).filename().string() + ".ff";
    }
    if ( !std::filesystem::exists( outputFile ) )
    {
        m_status = ASSET_OUT_OF_DATE;
    }

    Timestamp outTimestamp( outputFile );
    Timestamp newestFileTime( inputFile );

    m_status = outTimestamp <= newestFileTime ? ASSET_OUT_OF_DATE : ASSET_UP_TO_DATE;

    if ( m_status == ASSET_OUT_OF_DATE )
    {
        LOG( "OUT OF DATE: Fastfile file'", inputFile, "' has newer timestamp than saved FF" );
    }

    auto UpdateStatus = [this]( AssetStatus status ) {
        if ( status == ASSET_OUT_OF_DATE )
        {
            m_status = ASSET_OUT_OF_DATE;
        }
    };

    std::string line;
    while ( std::getline( in, line ) )
    {
        if ( line == "" || line[0] == '#' )
        {
            continue;
        }
        if ( line == "Shader" )
        {
            std::string tmp;
            ShaderConverter converter;
            if ( !ParseShaderCreateInfoFromFile( in, converter.createInfo ) )
            {
                LOG_ERR( "Error while parsing ShaderCreateInfo" );
                return ASSET_CHECKING_ERROR;
            }

            auto status = converter.CheckDependencies();
            if ( status == ASSET_CHECKING_ERROR )
            {
                LOG_ERR( "Error while checking dependencies on resource: '",
                         converter.createInfo.name, "'" );
                return ASSET_CHECKING_ERROR;
            }
            UpdateStatus( status );

            m_shaderConverters.emplace_back( std::move( converter ) );
        }
        else if ( line == "Texture" )
        {
            TextureConverter converter;
            if ( !ParseTextureCreateInfoFromFile( in, converter.createInfo ) )
            {
                LOG_ERR( "Error while parsing ShaderCreateInfo" );
                return ASSET_CHECKING_ERROR;
            }

            auto status = converter.CheckDependencies();
            if ( status == ASSET_CHECKING_ERROR )
            {
                LOG_ERR( "Error while checking dependencies on resource: '",
                         converter.createInfo.name, "'" );
                return ASSET_CHECKING_ERROR;
            }
            UpdateStatus( status );

            m_textureConverters.emplace_back( std::move( converter ) );
        }
        else if ( line == "MTLFile")
        {
            MaterialConverter converter;
            ParseMaterialFileFromFile( in, converter.inputFile );

            auto status = converter.CheckDependencies();
            if ( status == ASSET_CHECKING_ERROR )
            {
                LOG_ERR( "Error while checking dependencies on mtlFile '", converter.inputFile, "'" );
                return ASSET_CHECKING_ERROR;
            }
            UpdateStatus( status );

            m_materialFileConverters.emplace_back( std::move( converter ) );
        }
        
        else
        {
            LOG_WARN( "Unrecognized line: ", line );
        }
    }

    in.close();

    return m_status;
}

ConverterStatus FastfileConverter::Convert()
{
    if ( !force && m_status == ASSET_UP_TO_DATE )
    {
        return CONVERT_SUCCESS;
    }

    std::ofstream out( outputFile, std::ios::binary );

    if ( !out )
    {
        LOG_ERR( "Failed to open fastfile '", outputFile, "' for write" );
        return CONVERT_ERROR;
    }

    auto OnError = [&]()
    {
        out.close();
        std::filesystem::remove( outputFile );
        return CONVERT_ERROR;
    };

    uint32_t numShaders = m_shaderConverters.size();
    serialize::Write( out, numShaders );
    std::vector< char > buffer( 4 * 1024 * 1024 );
    for ( auto& converter : m_shaderConverters )
    {
        if ( converter.Convert() != CONVERT_SUCCESS )
        {
            LOG_ERR( "Error while converting shader" );
            return OnError();
        }

        // TODO: Dont write and then read back the same data, just write to both files
        std::ifstream in( converter.outputFile );
        if ( !in )
        {
            LOG_ERR( "Error opening intermediate file '", converter.outputFile, "'" );
            return OnError();
        }
        in.seekg( 0, in.end );
        size_t length = in.tellg();
        in.seekg( 0, in.beg );
        buffer.resize( length );
        in.read( &buffer[0], length );

        serialize::Write( out, buffer.data(), length );
    }

    uint32_t numTextures = m_textureConverters.size();
    serialize::Write( out, numTextures );
    for ( auto& converter : m_textureConverters )
    {
        if ( converter.Convert() != CONVERT_SUCCESS )
        {
            LOG_ERR( "Error while converting texture" );
            return OnError();
        }

        std::ifstream in( converter.outputFile );
        if ( !in )
        {
            LOG_ERR( "Error opening intermediate file '", converter.outputFile, "'" );
            return OnError();
        }
        in.seekg( 0, in.end );
        size_t length = in.tellg();
        in.seekg( 0, in.beg );
        buffer.resize( length );
        in.read( &buffer[0], length );

        serialize::Write( out, buffer.data(), length );
    }

    uint32_t numMaterials = m_materialFileConverters.size();
    serialize::Write( out, numMaterials );
    for ( auto& converter : m_materialFileConverters )
    {
        if ( converter.Convert() != CONVERT_SUCCESS )
        {
            LOG_ERR( "Error while converting material" );
            return OnError();
        }

        std::ifstream in( converter.outputFile );
        if ( !in )
        {
            LOG_ERR( "Error opening intermediate file '", converter.outputFile, "'" );
            return OnError();
        }
        in.seekg( 0, in.end );
        size_t length = in.tellg();
        in.seekg( 0, in.beg );
        buffer.resize( length );
        in.read( &buffer[0], length );

        serialize::Write( out, buffer.data(), length );
    }


    // uint32_t numModels = m_modelConverters.size();
    // serialize::Write( out, numModels );


    out.close();

    return CONVERT_SUCCESS;
}