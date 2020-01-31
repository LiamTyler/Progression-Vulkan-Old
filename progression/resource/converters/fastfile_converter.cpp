#include "core/feature_defines.hpp"
#include "basis_universal/transcoder/basisu_transcoder.h"
#include "basis_universal/basisu_comp.h"
#include "core/time.hpp"
#include "graphics/graphics_api.hpp"
#include "memory_map/MemoryMapped.h"
#include "resource/converters/fastfile_converter.hpp"
#include "resource/image.hpp"
#include "resource/model.hpp"
#include "resource/resource_version_numbers.hpp"
#include "resource/shader.hpp"
#include "utils/fileIO.hpp"
#include "utils/json_parsing.hpp"
#include "utils/logger.hpp"
#include "utils/lz4_compression.hpp"
#include "utils/serialize.hpp"
#include "utils/timestamp.hpp"
#include "utils/type_name.hpp"
#include <filesystem>
#include <fstream>

using namespace Progression;

static std::unordered_map< std::string, ImageSemantic > imageSemanticMap =
{
    { "DIFFUSE",    ImageSemantic::DIFFUSE },
    { "NORMAL",     ImageSemantic::NORMAL },
    { "METALLIC",   ImageSemantic::METALLIC },
    { "ROUGHNESS",  ImageSemantic::ROUGHNESS },
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
                i.dstFormat = Gfx::PixelFormatFromString( format );
                if ( i.dstFormat == Gfx::PixelFormat::INVALID && format != "INVALID" )
                {
                    LOG_WARN( "No pixel format found matching '", format, "'" );
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
#if USING( LZ4_COMPRESSED_FASTFILES )
    m_outputContentFile += "c";
#endif // #if USING( LZ4_COMPRESSED_FASTFILES )
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
static ConverterStatus WriteResources( std::ofstream& out, std::vector< ConverterType >& converters, uint32_t versionNumber, bool debugMode )
{
    uint32_t numResource = static_cast< uint32_t >( converters.size() );
    serialize::Write( out, numResource );
    serialize::Write( out, versionNumber );
    for ( auto& converter : converters )
    {
        if ( !converter.WriteToFastFile( out, debugMode ) )
        {
            return CONVERT_ERROR;
        }
    }
    serialize::Write( out, PG_RESOURCE_MAGIC_NUMBER_GUARD );
    return CONVERT_SUCCESS;
}

template< typename ConverterType >
static ConverterStatus RunConverts( std::vector< ConverterType >& converters )
{
    uint32_t numResource = static_cast< uint32_t >( converters.size() );
    for ( auto& converter : converters )
    {
        if ( converter.status != ASSET_UP_TO_DATE )
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
        }
    }

    return CONVERT_SUCCESS;
}

ConverterStatus FastfileConverter::Convert()
{
    if ( status == ASSET_UP_TO_DATE )
    {
        return CONVERT_SUCCESS;
    }

    auto fastFileStartTime = Time::GetTimePoint();

    ConverterStatus ret;
    ret = RunConverts( shaderConverters );
    if ( ret != CONVERT_SUCCESS ) return ret;

    ret = RunConverts( imageConverters );
    if ( ret != CONVERT_SUCCESS ) return ret;

    ret = RunConverts( materialFileConverters );
    if ( ret != CONVERT_SUCCESS ) return ret;

    ret = RunConverts( modelConverters );
    if ( ret != CONVERT_SUCCESS ) return ret;

    ret = RunConverts( scriptConverters );
    if ( ret != CONVERT_SUCCESS ) return ret;

    for ( int i = 0; i < 2; ++i )
    {
        std::string fname = i == 0 ? m_outputContentFile : m_outputContentFile + "d";
        bool debugMode    = i;
        std::ofstream out( fname, std::ios::binary );
        if ( !out )
        {
            LOG_ERR( "Failed to open fastfile '", fname, "' for write" );
            return CONVERT_ERROR;
        }

        std::string absPath = std::filesystem::absolute( inputFile ).string();
        serialize::Write( out, absPath );

        ConverterStatus ret;
        ret = WriteResources( out, shaderConverters, PG_RESOURCE_SHADER_VERSION, debugMode );
        if ( ret != CONVERT_SUCCESS ) return ret;

        ret = WriteResources( out, imageConverters, PG_RESOURCE_IMAGE_VERSION, debugMode );
        if ( ret != CONVERT_SUCCESS ) return ret;

        ret = WriteResources( out, materialFileConverters, PG_RESOURCE_MATERIAL_VERSION, debugMode );
        if ( ret != CONVERT_SUCCESS ) return ret;

        ret = WriteResources( out, modelConverters, PG_RESOURCE_MODEL_VERSION, debugMode );
        if ( ret != CONVERT_SUCCESS ) return ret;

        ret = WriteResources( out, scriptConverters, PG_RESOURCE_SCRIPT_VERSION, debugMode );
        if ( ret != CONVERT_SUCCESS ) return ret;

        out.close();

#if USING( LZ4_COMPRESSED_FASTFILES )
        LOG( "Compressing '", fname, "' with LZ4..." );
        if ( !LZ4CompressFile( fname, fname ) )
        {
            return CONVERT_ERROR;
        }
#endif // #if USING( LZ4_COMPRESSED_FASTFILES )
    }

    PG_MAYBE_UNUSED( fastFileStartTime );
    LOG( "\nFastfiles built in: ", Time::GetDuration( fastFileStartTime ) / 1000, " seconds" );

    return CONVERT_SUCCESS;
}

void FastfileConverter::UpdateStatus( AssetStatus s )
{
    if ( s > status )
    {
        status = s;
    }
}
