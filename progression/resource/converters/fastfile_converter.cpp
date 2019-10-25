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
    fileIO::ParseLineKeyValOptional( in, "filename", info.filename);
    info.filename = PG_RESOURCE_DIR + info.filename;

    return true;
}

static std::unordered_map< std::string, Gfx::PixelFormat > pixelFormatMap =
{
    { "R8_UNORM", Gfx::PixelFormat::R8_UNORM },
    { "R8_SNORM", Gfx::PixelFormat::R8_SNORM },
    { "R8_UINT", Gfx::PixelFormat::R8_UINT },
    { "R8_SINT", Gfx::PixelFormat::R8_SINT },
    { "R8_SRGB", Gfx::PixelFormat::R8_SRGB },
    { "R8_G8_UNORM", Gfx::PixelFormat::R8_G8_UNORM },
    { "R8_G8_SNORM", Gfx::PixelFormat::R8_G8_SNORM },
    { "R8_G8_UINT", Gfx::PixelFormat::R8_G8_UINT },
    { "R8_G8_SINT", Gfx::PixelFormat::R8_G8_SINT },
    { "R8_G8_SRGB", Gfx::PixelFormat::R8_G8_SRGB },
    { "R8_G8_B8_UNORM", Gfx::PixelFormat::R8_G8_B8_UNORM },
    { "R8_G8_B8_SNORM", Gfx::PixelFormat::R8_G8_B8_SNORM },
    { "R8_G8_B8_UINT", Gfx::PixelFormat::R8_G8_B8_UINT },
    { "R8_G8_B8_SINT", Gfx::PixelFormat::R8_G8_B8_SINT },
    { "R8_G8_B8_SRGB", Gfx::PixelFormat::R8_G8_B8_SRGB },
    { "R8_G8_B8_A8_UNORM", Gfx::PixelFormat::R8_G8_B8_A8_UNORM },
    { "R8_G8_B8_A8_SNORM", Gfx::PixelFormat::R8_G8_B8_A8_SNORM },
    { "R8_G8_B8_A8_UINT", Gfx::PixelFormat::R8_G8_B8_A8_UINT },
    { "R8_G8_B8_A8_SINT", Gfx::PixelFormat::R8_G8_B8_A8_SINT },
    { "R8_G8_B8_A8_SRGB", Gfx::PixelFormat::R8_G8_B8_A8_SRGB },
    { "R16_UNORM", Gfx::PixelFormat::R16_UNORM },
    { "R16_SNORM", Gfx::PixelFormat::R16_SNORM },
    { "R16_UINT", Gfx::PixelFormat::R16_UINT },
    { "R16_SINT", Gfx::PixelFormat::R16_SINT },
    { "R16_FLOAT", Gfx::PixelFormat::R16_FLOAT },
    { "R16_G16_UNORM", Gfx::PixelFormat::R16_G16_UNORM },
    { "R16_G16_SNORM", Gfx::PixelFormat::R16_G16_SNORM },
    { "R16_G16_UINT", Gfx::PixelFormat::R16_G16_UINT },
    { "R16_G16_SINT", Gfx::PixelFormat::R16_G16_SINT },
    { "R16_G16_FLOAT", Gfx::PixelFormat::R16_G16_FLOAT },
    { "R16_G16_B16_UNORM", Gfx::PixelFormat::R16_G16_B16_UNORM },
    { "R16_G16_B16_SNORM", Gfx::PixelFormat::R16_G16_B16_SNORM },
    { "R16_G16_B16_UINT", Gfx::PixelFormat::R16_G16_B16_UINT },
    { "R16_G16_B16_SINT", Gfx::PixelFormat::R16_G16_B16_SINT },
    { "R16_G16_B16_FLOAT", Gfx::PixelFormat::R16_G16_B16_FLOAT },
    { "R16_G16_B16_A16_UNORM", Gfx::PixelFormat::R16_G16_B16_A16_UNORM },
    { "R16_G16_B16_A16_SNORM", Gfx::PixelFormat::R16_G16_B16_A16_SNORM },
    { "R16_G16_B16_A16_UINT", Gfx::PixelFormat::R16_G16_B16_A16_UINT },
    { "R16_G16_B16_A16_SINT", Gfx::PixelFormat::R16_G16_B16_A16_SINT },
    { "R16_G16_B16_A16_FLOAT", Gfx::PixelFormat::R16_G16_B16_A16_FLOAT },
    { "R32_UINT", Gfx::PixelFormat::R32_UINT },
    { "R32_SINT", Gfx::PixelFormat::R32_SINT },
    { "R32_FLOAT", Gfx::PixelFormat::R32_FLOAT },
    { "R32_G32_UINT", Gfx::PixelFormat::R32_G32_UINT },
    { "R32_G32_SINT", Gfx::PixelFormat::R32_G32_SINT },
    { "R32_G32_FLOAT", Gfx::PixelFormat::R32_G32_FLOAT },
    { "R32_G32_B32_UINT", Gfx::PixelFormat::R32_G32_B32_UINT },
    { "R32_G32_B32_SINT", Gfx::PixelFormat::R32_G32_B32_SINT },
    { "R32_G32_B32_FLOAT", Gfx::PixelFormat::R32_G32_B32_FLOAT },
    { "R32_G32_B32_A32_UINT", Gfx::PixelFormat::R32_G32_B32_A32_UINT },
    { "R32_G32_B32_A32_SINT", Gfx::PixelFormat::R32_G32_B32_A32_SINT },
    { "R32_G32_B32_A32_FLOAT", Gfx::PixelFormat::R32_G32_B32_A32_FLOAT },
    { "DEPTH_16_UNORM", Gfx::PixelFormat::DEPTH_16_UNORM },
    { "DEPTH_32_FLOAT", Gfx::PixelFormat::DEPTH_32_FLOAT },
    { "DEPTH_16_UNORM_STENCIL_8_UINT", Gfx::PixelFormat::DEPTH_16_UNORM_STENCIL_8_UINT },
    { "DEPTH_24_UNORM_STENCIL_8_UINT", Gfx::PixelFormat::DEPTH_24_UNORM_STENCIL_8_UINT },
    { "DEPTH_32_FLOAT_STENCIL_8_UINT", Gfx::PixelFormat::DEPTH_32_FLOAT_STENCIL_8_UINT },
    { "STENCIL_8_UINT", Gfx::PixelFormat::STENCIL_8_UINT },
    { "BC1_RGB_UNORM", Gfx::PixelFormat::BC1_RGB_UNORM },
    { "BC1_RGB_SRGB", Gfx::PixelFormat::BC1_RGB_SRGB },
    { "BC1_RGBA_UNORM", Gfx::PixelFormat::BC1_RGBA_UNORM },
    { "BC1_RGBA_SRGB", Gfx::PixelFormat::BC1_RGBA_SRGB },
    { "BC2_UNORM", Gfx::PixelFormat::BC2_UNORM },
    { "BC2_SRGB", Gfx::PixelFormat::BC2_SRGB },
    { "BC3_UNORM", Gfx::PixelFormat::BC3_UNORM },
    { "BC3_SRGB", Gfx::PixelFormat::BC3_SRGB },
    { "BC4_UNORM", Gfx::PixelFormat::BC4_UNORM },
    { "BC4_SNORM", Gfx::PixelFormat::BC4_SNORM },
    { "BC5_UNORM", Gfx::PixelFormat::BC5_UNORM },
    { "BC5_SNORM", Gfx::PixelFormat::BC5_SNORM },
    { "BC6H_UFLOAT", Gfx::PixelFormat::BC6H_UFLOAT },
    { "BC6H_SFLOAT", Gfx::PixelFormat::BC6H_SFLOAT },
    { "BC7_UNORM", Gfx::PixelFormat::BC7_UNORM },
    { "BC7_SRGB", Gfx::PixelFormat::BC7_SRGB },
};

bool ParseImageCreateInfoFromFile( std::istream& in, ImageCreateInfo& info )
{
    fileIO::ParseLineKeyVal( in, "name",     info.name );
    fileIO::ParseLineKeyVal( in, "filenames", info.filenames );
    for ( auto& fname : info.filenames )
    {
        fname = PG_RESOURCE_DIR + fname;
    }
    // auto res = fileIO::ParseLineKeyMap( in, "dstFormat", pixelFormatMap, info.dstFormat ), "Check pixel format for spelling mistake" 
    // PG_ASSERT( res, "Check pixel format for spelling mistake" );
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

bool ParseScriptFileFromFile( std::istream& in, ScriptCreateInfo& createInfo )
{
    fileIO::ParseLineKeyVal( in, "name", createInfo.name );
    fileIO::ParseLineKeyVal( in, "filename", createInfo.filename );
    createInfo.filename = PG_RESOURCE_DIR + createInfo.filename;
    return true;
}

AssetStatus FastfileConverter::CheckDependencies()
{
    IF_VERBOSE_MODE( LOG( "Checking dependencies for fastfile '", inputFile, "'" ) );
    std::ifstream in( inputFile );
    if ( !in )
    {
        LOG_ERR( "Could not open the input resource file: '", inputFile, "'" );
        return ASSET_CHECKING_ERROR;
    }

    m_status = ASSET_UP_TO_DATE;

    m_outputContentFile = PG_RESOURCE_DIR "cache/fastfiles/" +
                          std::filesystem::path( inputFile ).filename().string() + ".ff";
    IF_VERBOSE_MODE( LOG( "Resource file '", inputFile, "' outputs fastfile '", m_outputContentFile, "'" ) );
    if ( !std::filesystem::exists( m_outputContentFile ) )
    {
        LOG( "OUT OF DATE: Fastfile file for '", inputFile, "' does not exist, convert required" );
        m_status = ASSET_OUT_OF_DATE;
    }
    else
    {
        Timestamp outTimestamp( m_outputContentFile );
        Timestamp newestFileTime( inputFile );

        IF_VERBOSE_MODE( LOG( "Resource file timestamp: ", newestFileTime, ", fastfile timestamp: ", outTimestamp ) );

        m_status = outTimestamp <= newestFileTime ? ASSET_OUT_OF_DATE : ASSET_UP_TO_DATE;

        if ( m_status == ASSET_OUT_OF_DATE )
        {
            LOG( "OUT OF DATE: Fastfile file '", inputFile, "' has newer timestamp than saved FF" );
        }
    }

    auto UpdateStatus = [this]( AssetStatus status )
    {
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
            ShaderConverter converter;
            converter.force   = force;
            converter.verbose = verbose;
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
            converter.force   = force;
            converter.verbose = verbose;
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
            converter.force   = force;
            converter.verbose = verbose;
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
            converter.force   = force;
            converter.verbose = verbose;
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
        else if ( line == "Script")
        {
            ScriptConverter converter;
            converter.force   = force;
            converter.verbose = verbose;
            ParseScriptFileFromFile( in, converter.createInfo );

            auto status = converter.CheckDependencies();
            if ( status == ASSET_CHECKING_ERROR )
            {
                LOG_ERR( "Error while checking dependencies on script file '", converter.createInfo.filename, "'" );
                return ASSET_CHECKING_ERROR;
            }
            UpdateStatus( status );

            m_scriptConverters.emplace_back( std::move( converter ) );
        }
        else
        {
            LOG_WARN( "Unrecognized line: ", line );
        }
    }

    in.close();

    if ( force && ASSET_UP_TO_DATE )
    {
        LOG( "Fastfile '", inputFile, "' is up to date, but --force used, so converting anyways\n" );
        m_status = ASSET_OUT_OF_DATE;
    }

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
                PG_MAYBE_UNUSED( time ); \
                LOG( "Convert finished in: ", Time::GetDuration( time ) / 1000, " seconds" ); \
                converter.WriteToFastFile( out ); \
            } \
        } \
    }

    WRITE_RESOURCE( Shader, m_shaderConverters )
    WRITE_RESOURCE( Image, m_imageConverters )
    WRITE_RESOURCE( MTLFile, m_materialFileConverters )
    WRITE_RESOURCE( Model, m_modelConverters )
    WRITE_RESOURCE( Script, m_scriptConverters )

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

    PG_MAYBE_UNUSED( fastFileStartTime );
    LOG( "\nFastfile built in: ", Time::GetDuration( fastFileStartTime ) / 1000, " seconds" );

    return CONVERT_SUCCESS;
}
