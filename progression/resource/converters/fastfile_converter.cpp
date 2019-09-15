#include "core/feature_defines.hpp"
#include "core/time.hpp"
#include "graphics/graphics_api.hpp"
#include "lz4/lz4.h"
#include "memory_map/MemoryMapped.h"
#include "resource/converters/fastfile_converter.hpp"
#include "resource/image.hpp"
#include "resource/model.hpp"
#include "resource/shader.hpp"
#include "utils/fileIO.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#include "utils/timestamp.hpp"
#include <filesystem>
#include <fstream>

using namespace Progression;

bool ParseShaderCreateInfoFromFile( std::istream& in, ShaderCreateInfo& info )
{
    fileIO::ParseLineKeyVal( in, "name", info.name );
    fileIO::ParseLineKeyValOptional( in, "vertex", info.vertex );
    fileIO::ParseLineKeyValOptional( in, "geometry", info.geometry );
    fileIO::ParseLineKeyValOptional( in, "fragment", info.fragment );
    fileIO::ParseLineKeyValOptional( in, "compute", info.compute );
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

/*
static std::unordered_map< std::string, Gfx::PixelFormat > internalFormatMap = {
    { "R8_Uint",                Gfx::PixelFormat::R8_Uint },
    { "R16_Float",              Gfx::PixelFormat::R16_Float },
    { "R32_Float",              Gfx::PixelFormat::R32_Float },
    { "R8_G8_Uint",             Gfx::PixelFormat::R8_G8_Uint },
    { "R16_G16_Float",          Gfx::PixelFormat::R16_G16_Float },
    { "R32_G32_Float",          Gfx::PixelFormat::R32_G32_Float },
    { "R8_G8_B8_Uint",          Gfx::PixelFormat::R8_G8_B8_Uint },
    { "R16_G16_B16_Float",      Gfx::PixelFormat::R16_G16_B16_Float },
    { "R32_G32_B32_Float",      Gfx::PixelFormat::R32_G32_B32_Float },
    { "R8_G8_B8_A8_Uint",       Gfx::PixelFormat::R8_G8_B8_A8_Uint },
    { "R16_G16_B16_A16_Float",  Gfx::PixelFormat::R16_G16_B16_A16_Float },
    { "R32_G32_B32_A32_Float",  Gfx::PixelFormat::R32_G32_B32_A32_Float },
    { "R8_G8_B8_Uint_sRGB",     Gfx::PixelFormat::R8_G8_B8_Uint_sRGB },
    { "R8_G8_B8_A8_Uint_sRGB",  Gfx::PixelFormat::R8_G8_B8_A8_Uint_sRGB },
    { "R11_G11_B10_Float",      Gfx::PixelFormat::R11_G11_B10_Float },
    { "DEPTH32_Float",          Gfx::PixelFormat::DEPTH32_Float },
};
*/

bool ParseImageCreateInfoFromFile( std::istream& in, ImageCreateInfo& info )
{
    fileIO::ParseLineKeyVal( in, "name",     info.name );
    fileIO::ParseLineKeyVal( in, "filenames", info.filenames );
    for ( auto& fname : info.filenames )
    {
        fname = PG_RESOURCE_DIR + fname;
    }
    info.flags    = static_cast< ImageFlags >( 0 );
    fileIO::ParseLineKeyValOptional( in, "sampler", info.sampler );

    bool tmp;
    fileIO::ParseLineKeyVal( in, "flipVertically", tmp );
    if ( tmp )
    {
        info.flags |= IMAGE_FLIP_VERTICALLY;
    }
    fileIO::ParseLineKeyVal( in, "generateMipmaps", tmp );
    if ( tmp )
    {
        info.flags |= IMAGE_GENERATE_MIPMAPS_ON_CONVERT;
    }
    fileIO::ParseLineKeyVal( in, "createTexture", tmp );
    if ( tmp )
    {
        info.flags |= IMAGE_CREATE_TEXTURE_ON_LOAD;
    }
    fileIO::ParseLineKeyVal( in, "freeCpuCopy", tmp );
    if ( tmp )
    {
        info.flags |= IMAGE_FREE_CPU_COPY_ON_LOAD;
    }

    return true;
}

bool ParseMaterialFileFromFile( std::istream& in, std::string& mtlFileName )
{
    fileIO::ParseLineKeyVal( in, "filename", mtlFileName );
    mtlFileName = PG_RESOURCE_DIR + mtlFileName;
    return true;
}

bool ParseModelFromFile( std::istream& in, ModelCreateInfo& createInfo )
{
    fileIO::ParseLineKeyVal( in, "name",          createInfo.name );
    fileIO::ParseLineKeyVal( in, "filename",      createInfo.filename );
    fileIO::ParseLineKeyVal( in, "optimize",      createInfo.optimize );
    fileIO::ParseLineKeyVal( in, "freeCpuCopy",   createInfo.freeCpuCopy );
    fileIO::ParseLineKeyVal( in, "createGpuCopy", createInfo.createGpuCopy );
    createInfo.filename = PG_RESOURCE_DIR + createInfo.filename;
    
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

    m_outputContentFile = PG_RESOURCE_DIR "cache/fastfiles/" +
                          std::filesystem::path( inputFile ).filename().string() + ".ff";
    if ( !std::filesystem::exists( m_outputContentFile ) )
    {
        LOG( "OUT OF DATE: Fastfile file for '", inputFile, "' does not exist, convert required" );
        m_status = ASSET_OUT_OF_DATE;
    }
    else
    {
        Timestamp outTimestamp( m_outputContentFile );
        Timestamp newestFileTime( inputFile );

        m_status = outTimestamp <= newestFileTime ? ASSET_OUT_OF_DATE : ASSET_UP_TO_DATE;

        if ( m_status == ASSET_OUT_OF_DATE )
        {
            LOG( "OUT OF DATE: Fastfile file '", inputFile, "' has newer timestamp than saved FF" );
        }
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
                LOG_ERR( "Error while checking dependencies on resource: '", converter.createInfo.name, "'" );
                return ASSET_CHECKING_ERROR;
            }
            UpdateStatus( status );

            m_shaderConverters.emplace_back( std::move( converter ) );
        }
        else if ( line == "Image" )
        {
            ImageConverter converter;
            if ( !ParseImageCreateInfoFromFile( in, converter.createInfo ) )
            {
                LOG_ERR( "Error while parsing ShaderCreateInfo" );
                return ASSET_CHECKING_ERROR;
            }

            auto status = converter.CheckDependencies();
            if ( status == ASSET_CHECKING_ERROR )
            {
                LOG_ERR( "Error while checking dependencies on resource: '", converter.createInfo.name, "'" );
                return ASSET_CHECKING_ERROR;
            }
            UpdateStatus( status );

            m_imageConverters.emplace_back( std::move( converter ) );
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
        else if ( line == "Model")
        {
            ModelConverter converter;
            ParseModelFromFile( in, converter.createInfo );

            auto status = converter.CheckDependencies();
            if ( status == ASSET_CHECKING_ERROR )
            {
                LOG_ERR( "Error while checking dependencies on model file '", converter.createInfo.filename, "'" );
                return ASSET_CHECKING_ERROR;
            }
            UpdateStatus( status );

            m_modelConverters.emplace_back( std::move( converter ) );
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
    if ( m_status == ASSET_UP_TO_DATE )
    {
        return CONVERT_SUCCESS;
    }

    auto fastFileStartTime = Time::GetTimePoint();

    std::ofstream out( m_outputContentFile, std::ios::binary );

    if ( !out )
    {
        LOG_ERR( "Failed to open fastfile '", m_outputContentFile, "' for write" );
        return CONVERT_ERROR;
    }

    std::vector< char > buffer( 4 * 1024 * 1024 );

#define WRITE_RESOURCE( resource, converterList ) \
    { \
        uint32_t numResource = static_cast< uint32_t >( converterList.size() ); \
        serialize::Write( out, numResource ); \
        for ( auto& converter : converterList ) \
        { \
            if ( converter.GetStatus() == ASSET_UP_TO_DATE ) { \
                converter.WriteToFastFile( out ); \
            } \
            else \
            { \
                auto time = Time::GetTimePoint(); \
                LOG( "\nConverting ", #resource, " '", converter.GetName(), "'" ); \
                if ( converter.Convert() != CONVERT_SUCCESS ) \
                { \
                    LOG_ERR( "Error while converting " #resource ); \
                    return CONVERT_ERROR; \
                } \
                LOG( "Convert finished in: ", Time::GetDuration( time ) / 1000, " seconds" ); \
                converter.WriteToFastFile( out ); \
            } \
        } \
    }

    WRITE_RESOURCE( Shader, m_shaderConverters )
    WRITE_RESOURCE( Image, m_imageConverters )
    WRITE_RESOURCE( MTLFile, m_materialFileConverters )
    WRITE_RESOURCE( Model, m_modelConverters )

    out.close();

#if USING( LZ4_COMPRESSED_FASTFILES )
    LOG( "Compressing with LZ4..." );
    MemoryMapped memMappedFile;
    if ( !memMappedFile.open( outputContentFile, MemoryMapped::WholeFile, MemoryMapped::Normal ) )
    {
        LOG_ERR( "Could not open fastfile:", outputContentFile );
        return CONVERT_ERROR;
    }

    char* src = (char*) memMappedFile.getData();
    const int srcSize = (int) memMappedFile.size();

    const int maxDstSize = LZ4_compressBound( srcSize );

    char* compressedData = (char*) malloc( maxDstSize );
    const int compressedDataSize = LZ4_compress_default( src, compressedData, srcSize, maxDstSize );

    memMappedFile.close();

    if ( compressedDataSize <= 0 )
    {
        LOG_ERR("Error while trying to compress the fastfile. LZ4 returned: ", compressedDataSize );
        return CONVERT_ERROR;
    }

    if ( compressedDataSize > 0 )
    {
        LOG( "Compressed file size ratio: ", (float) compressedDataSize / srcSize );
    }

    out.open( outputContentFile, std::ios::binary );

    if ( !out )
    {
        LOG_ERR( "Failed to open fastfile '", outputContentFile, "' for writing compressed results" );
        return CONVERT_ERROR;
    }

    serialize::Write( out, srcSize );
    serialize::Write( out, compressedData, compressedDataSize );

    out.close();
#endif // #if USING( LZ4_COMPRESSED_FASTFILES )

    LOG( "\nFastfile built in: ", Time::GetDuration( fastFileStartTime ) / 1000, " seconds" );

    return CONVERT_SUCCESS;
}
