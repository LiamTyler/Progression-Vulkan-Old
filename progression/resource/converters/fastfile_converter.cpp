#include "core/feature_defines.hpp"
#include "basis_universal/transcoder/basisu_transcoder.h"
#include "basis_universal/basisu_comp.h"
#include "core/time.hpp"
#include "graphics/graphics_api.hpp"
#include "lz4/lz4.h"
#include "memory_map/MemoryMapped.h"
#include "resource/converters/fastfile_converter.hpp"
#include "resource/image.hpp"
#include "resource/model.hpp"
#include "resource/resource_version_numbers.hpp"
#include "resource/shader.hpp"
#include "utils/fileIO.hpp"
#include "utils/json_parsing.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#include "utils/timestamp.hpp"
#include "utils/type_name.hpp"
#include <filesystem>
#include <fstream>

using namespace Progression;

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

static std::unordered_map< std::string, ImageSemantic > imageSemanticMap =
{
    { "DIFFUSE", ImageSemantic::DIFFUSE },
    { "NORMAL",  ImageSemantic::NORMAL }
};

static std::unordered_map< std::string, ImageCompressionQuality > imageCompressionQualityMap =
{
    { "LOW",    ImageCompressionQuality::LOW },
    { "MEDIUM", ImageCompressionQuality::MEDIUM },
    { "HIGH",   ImageCompressionQuality::HIGH },
    { "MAX",    ImageCompressionQuality::MAX },
};

static void ParseImage( rapidjson::Value& value, FastfileConverter* conv )
{
    static FunctionMapper< void, std::vector< std::string >& > cubemapParser(
    {
        { "right",  []( rapidjson::Value& v, std::vector< std::string >& files ) { files[0] = v.GetString(); } },
        { "left",   []( rapidjson::Value& v, std::vector< std::string >& files ) { files[1] = v.GetString(); } },
        { "top",    []( rapidjson::Value& v, std::vector< std::string >& files ) { files[3] = v.GetString(); } },
        { "bottom", []( rapidjson::Value& v, std::vector< std::string >& files ) { files[2] = v.GetString(); } },
        { "back",   []( rapidjson::Value& v, std::vector< std::string >& files ) { files[4] = v.GetString(); } },
        { "front",  []( rapidjson::Value& v, std::vector< std::string >& files ) { files[5] = v.GetString(); } },
    });

    ImageConverter converter( conv->force, conv->verbose );
    static FunctionMapper< void, ImageCreateInfo& > mapping(
    {
        { "name",             []( rapidjson::Value& v, ImageCreateInfo& i ) { i.name     = v.GetString(); } },
        { "filename",         []( rapidjson::Value& v, ImageCreateInfo& i ) { i.filename = PG_RESOURCE_DIR + std::string( v.GetString() ); } },
        { "cubeMapFilenames", []( rapidjson::Value& v, ImageCreateInfo& i )
            {
                i.cubeMapFilenames.clear();
                i.cubeMapFilenames.resize( 6 );
                cubemapParser.ForEachMember( v, i.cubeMapFilenames );
                for ( auto& file : i.cubeMapFilenames )
                {
                    PG_ASSERT( file != "", "Please specify all 6 faces for cubemap" );
                    file = PG_RESOURCE_DIR + file;
                }
            }
        },
        { "semantic", []( rapidjson::Value& v, ImageCreateInfo& i )
            {
                std::string semanticName = v.GetString();
                auto it = imageSemanticMap.find( semanticName );
                if ( it == imageSemanticMap.end() )
                {
                    LOG_WARN( "No image semantic found matching '", semanticName, "'" );
                }
                else
                {
                    i.semantic = it->second;
                }
            }
        },
        { "sampler",         []( rapidjson::Value& v, ImageCreateInfo& i ) { i.sampler = v.GetString(); } },
        { "flipVertically",  []( rapidjson::Value& v, ImageCreateInfo& i ) { if ( v.GetBool() ) i.flags |= IMAGE_FLIP_VERTICALLY; } },
        { "generateMipmaps", []( rapidjson::Value& v, ImageCreateInfo& i ) { if ( v.GetBool() ) i.flags |= IMAGE_GENERATE_MIPMAPS; } },
        { "createTexture",   []( rapidjson::Value& v, ImageCreateInfo& i ) { if ( v.GetBool() ) i.flags |= IMAGE_CREATE_TEXTURE_ON_LOAD; } },
        { "freeCpuCopy",     []( rapidjson::Value& v, ImageCreateInfo& i ) { if ( v.GetBool() ) i.flags |= IMAGE_FREE_CPU_COPY_ON_LOAD; } },
        { "dstFormat",       []( rapidjson::Value& v, ImageCreateInfo& i )
            {
                std::string format = v.GetString();
                auto it = pixelFormatMap.find( format );
                if ( it == pixelFormatMap.end() )
                {
                    LOG_WARN( "No pixel format found matching '", format, "'" );
                }
                else
                {
                    i.dstFormat = it->second;
                }
            }
        },
        { "compressionQuality", []( rapidjson::Value& v, ImageCreateInfo& i )
            {
                std::string s = v.GetString();
                auto it = imageCompressionQualityMap.find( s );
                if ( it == imageCompressionQualityMap.end() )
                {
                    LOG_WARN( "No image compression quality found matching '", s, "'" );
                }
                else
                {
                    i.compressionQuality = it->second;
                }
            }
        },
    });

    mapping.ForEachMember( value, converter.createInfo );
    
    auto status = converter.CheckDependencies();
    conv->UpdateStatus( status );
    if ( status == ASSET_CHECKING_ERROR )
    {
        LOG_ERR( "Error while checking dependencies on script: '", converter.createInfo.name, "'" );
        return;
    }
    conv->imageConverters.emplace_back( std::move( converter ) );
}

static void ParseModel( rapidjson::Value& value, FastfileConverter* conv )
{
    ModelConverter converter( conv->force, conv->verbose );
    static FunctionMapper< void, ModelCreateInfo& > mapping(
    {
        { "name",          []( rapidjson::Value& v, ModelCreateInfo& i ) { i.name          = v.GetString(); } },
        { "filename",      []( rapidjson::Value& v, ModelCreateInfo& i ) { i.filename      = PG_RESOURCE_DIR + std::string( v.GetString() ); } },
        { "optimize",      []( rapidjson::Value& v, ModelCreateInfo& i ) { i.optimize      = v.GetBool(); } },
        { "freeCpuCopy",   []( rapidjson::Value& v, ModelCreateInfo& i ) { i.freeCpuCopy   = v.GetBool(); } },
        { "createGpuCopy", []( rapidjson::Value& v, ModelCreateInfo& i ) { i.createGpuCopy = v.GetBool(); } },
    });

    mapping.ForEachMember( value, converter.createInfo );

    auto status = converter.CheckDependencies();
    conv->UpdateStatus( status );
    if ( status == ASSET_CHECKING_ERROR )
    {
        LOG_ERR( "Error while checking dependencies on script: '", converter.createInfo.name, "'" );
        return;
    }
    conv->modelConverters.emplace_back( std::move( converter ) );
}

static void ParseMTLFile( rapidjson::Value& value, FastfileConverter* conv )
{
    MaterialConverter converter( conv->force, conv->verbose );
    static FunctionMapper< void, std::string& > mapping(
    {
        { "filename", []( rapidjson::Value& v, std::string& f ) { f = PG_RESOURCE_DIR + std::string( v.GetString() ); } },
    });

    mapping.ForEachMember( value, converter.inputFile );

    auto status = converter.CheckDependencies();
    conv->UpdateStatus( status );
    if ( status == ASSET_CHECKING_ERROR )
    {
        LOG_ERR( "Error while checking dependencies on MTLFile: '", converter.inputFile, "'" );
        return;
    }
    conv->materialFileConverters.emplace_back( std::move( converter ) );
}

static void ParseScript( rapidjson::Value& value, FastfileConverter* conv )
{
    ScriptConverter converter( conv->force, conv->verbose );
    static FunctionMapper< void, ScriptCreateInfo& > mapping(
    {
        { "name",     []( rapidjson::Value& v, ScriptCreateInfo& i ) { i.name     = v.GetString(); } },
        { "filename", []( rapidjson::Value& v, ScriptCreateInfo& i ) { i.filename = PG_RESOURCE_DIR + std::string( v.GetString() ); } },
    });

    mapping.ForEachMember( value, converter.createInfo );
    
    auto status = converter.CheckDependencies();
    conv->UpdateStatus( status );
    if ( status == ASSET_CHECKING_ERROR )
    {
        LOG_ERR( "Error while checking dependencies on script: '", converter.createInfo.name, "'" );
        return;
    }
    conv->scriptConverters.emplace_back( std::move( converter ) );
}

static void ParseShader( rapidjson::Value& value, FastfileConverter* conv )
{
    ShaderConverter converter( conv->force, conv->verbose );
    static FunctionMapper< void, ShaderCreateInfo& > mapping(
    {
        { "name",     []( rapidjson::Value& v, ShaderCreateInfo& i ) { i.name     = v.GetString(); } },
        { "filename", []( rapidjson::Value& v, ShaderCreateInfo& i ) { i.filename = PG_RESOURCE_DIR + std::string( v.GetString() ); } },
    });

    mapping.ForEachMember( value, converter.createInfo );

    auto status = converter.CheckDependencies();
    conv->UpdateStatus( status );
    if ( status == ASSET_CHECKING_ERROR )
    {
        LOG_ERR( "Error while checking dependencies on shader: '", converter.createInfo.name, "'" );
        return;
    }
    conv->shaderConverters.emplace_back( std::move( converter ) );
}

AssetStatus FastfileConverter::CheckDependencies()
{
    basisu::basisu_encoder_init();
    basist::basisu_transcoder_init();
    IF_VERBOSE_MODE( LOG( "Checking dependencies for fastfile '", inputFile, "'" ) );
    status = ASSET_UP_TO_DATE;

    m_outputContentFile = PG_RESOURCE_DIR "cache/fastfiles/" + std::filesystem::path( inputFile ).stem().string() + ".ff";
    IF_VERBOSE_MODE( LOG( "Resource file '", inputFile, "' outputs fastfile '", m_outputContentFile, "'" ) );
    if ( !std::filesystem::exists( m_outputContentFile ) )
    {
        LOG( "OUT OF DATE: Fastfile file for '", inputFile, "' does not exist, convert required" );
        status = ASSET_OUT_OF_DATE;
    }
    else
    {
        Timestamp outTimestamp( m_outputContentFile );
        Timestamp newestFileTime( inputFile );

        IF_VERBOSE_MODE( LOG( "Resource file timestamp: ", newestFileTime, ", fastfile timestamp: ", outTimestamp ) );

        status = outTimestamp <= newestFileTime ? ASSET_OUT_OF_DATE : ASSET_UP_TO_DATE;

        if ( status == ASSET_OUT_OF_DATE )
        {
            LOG( "OUT OF DATE: Fastfile file '", inputFile, "' has newer timestamp than saved FF" );
        }
    }

    auto document = ParseJSONFile( inputFile );
    if ( document.IsNull() )
    {
        return ASSET_CHECKING_ERROR;
    }

    static FunctionMapper< void, FastfileConverter* > mapping(
    {
        { "Image",   ParseImage },
        { "Model",   ParseModel },
        { "MTLFile", ParseMTLFile },
        { "Script",  ParseScript },
        { "Shader",  ParseShader },
    });

    mapping.ForEachMember( document, std::move( this ) );

    if ( force && status == ASSET_UP_TO_DATE )
    {
        LOG( "Fastfile '", inputFile, "' is up to date, but --force used, so converting anyways\n" );
        status = ASSET_OUT_OF_DATE;
    }

    return status;
}

template< typename ConverterType >
static ConverterStatus WriteResources( std::ofstream& out, std::vector< ConverterType >& converters, uint32_t versionNumber )
{
    uint32_t numResource = static_cast< uint32_t >( converters.size() );
    serialize::Write( out, numResource );
    serialize::Write( out, versionNumber );
    for ( auto& converter : converters )
    {
        if ( converter.status == ASSET_UP_TO_DATE )
        {
            converter.WriteToFastFile( out );
        }
        else
        {
            auto time = Time::GetTimePoint();
            LOG( "\nConverter: ", type_name< ConverterType >(), ", resource '", converter.GetName(), "'" );
            if ( converter.Convert() != CONVERT_SUCCESS )
            {
                LOG_ERR( "Error while in ", type_name< ConverterType >() );
                return CONVERT_ERROR;
            }
            PG_MAYBE_UNUSED( time );
            LOG( "Convert finished in: ", Time::GetDuration( time ) / 1000, " seconds" );
            converter.WriteToFastFile( out );
        }
    }
    serialize::Write( out, PG_RESOURCE_MAGIC_NUMBER_GUARD );
    return CONVERT_SUCCESS;
}

ConverterStatus FastfileConverter::Convert()
{
    if ( status == ASSET_UP_TO_DATE )
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

    std::string absPath = std::filesystem::absolute( inputFile ).string();
    serialize::Write( out, absPath );

    ConverterStatus ret;
    ret = WriteResources( out, shaderConverters, PG_RESOURCE_SHADER_VERSION );
    if ( ret != CONVERT_SUCCESS ) return ret;

    ret = WriteResources( out, imageConverters, PG_RESOURCE_IMAGE_VERSION );
    if ( ret != CONVERT_SUCCESS ) return ret;

    ret = WriteResources( out, materialFileConverters, PG_RESOURCE_MATERIAL_VERSION );
    if ( ret != CONVERT_SUCCESS ) return ret;

    ret = WriteResources( out, modelConverters, PG_RESOURCE_MODEL_VERSION );
    if ( ret != CONVERT_SUCCESS ) return ret;

    ret = WriteResources( out, scriptConverters, PG_RESOURCE_SCRIPT_VERSION );
    if ( ret != CONVERT_SUCCESS ) return ret;

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

void FastfileConverter::UpdateStatus( AssetStatus s )
{
    if ( s > status )
    {
        status = s;
    }
}
