#include "resource/converters/model_converter.hpp"
#include "core/assert.hpp"
#include "resource/model.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#include "utils/timestamp.hpp"
#include <filesystem>
#include <fstream>

using namespace Progression;

static std::string GetModelFastFileName( struct ModelCreateInfo& createInfo )
{
    namespace fs = std::filesystem;

    PG_ASSERT( !createInfo.filename.empty() );
    fs::path filePath = fs::absolute( createInfo.filename );

    size_t hash          = std::hash< std::string >{}( filePath.string() );
    std::string baseName = filePath.filename().string();

    return PG_RESOURCE_DIR "cache/models/" + baseName + "_" + std::to_string( hash ) + ".ffi";
}

AssetStatus ModelConverter::CheckDependencies()
{
    PG_ASSERT( !createInfo.filename.empty() );

    if ( outputFile.empty() )
    {
        outputFile = GetModelFastFileName( createInfo );
    }

    if ( !std::filesystem::exists( outputFile ) )
    {
        LOG( "OUT OF DATE: FFI File for Model '", createInfo.filename, "' does not exist, convert required" );
        return ASSET_OUT_OF_DATE;
    }

    namespace fs = std::filesystem;
    auto path = fs::path( createInfo.filename );
    std::string matFile = path.parent_path().string() + "/" + path.stem().string() + ".mtl";
    Timestamp outTimestamp( outputFile );
    Timestamp mtlTimestamp( matFile );
    Timestamp objFileTime( createInfo.filename );

    m_status = outTimestamp <= objFileTime || outTimestamp <= mtlTimestamp ? ASSET_OUT_OF_DATE : ASSET_UP_TO_DATE;

    if ( outTimestamp <= mtlTimestamp )
    {
        LOG( "OUT OF DATE: MTL file '", matFile, "' for model '", createInfo.filename, "' has newer timestamp than saved FFI" );
    }

    if ( outTimestamp <= objFileTime )
    {
        LOG( "OUT OF DATE: Model file '", createInfo.filename, "' has newer timestamp than saved FFI" );
    }
    
    if ( m_status == ASSET_UP_TO_DATE )
    {
        LOG( "UP TO DATE: Model file '", createInfo.filename, "' and MTL file '", matFile, "'" );
    }

    return m_status;
}

ConverterStatus ModelConverter::Convert()
{
    if ( m_status == ASSET_UP_TO_DATE )
    {
        return CONVERT_SUCCESS;
    }

    Model model;

    createInfo.freeCpuCopy = false;
    createInfo.optimize    = true;
    if ( !model.Load( &createInfo ) )
    {
        LOG_ERR( "Could not load the model '", createInfo.filename, "'" );
        return CONVERT_ERROR;
    }

    std::ofstream out( outputFile, std::ios::binary );

    if ( !model.Serialize( out ) )
    {
        LOG_ERR( "Could not save the model '", createInfo.filename, "' to fastfile" );
        return CONVERT_ERROR;
    }

    out.close();

    return CONVERT_SUCCESS;
}
