#include "resource/converters/model_converter.hpp"
#include "resource/model.hpp"
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

    size_t hash          = std::hash< std::string >{}( filePath );
    std::string baseName = filePath.filename();

    return PG_RESOURCE_DIR "cache/models/" + baseName + std::to_string( hash ) + ".ffi";
}

AssetStatus ModelConverter::CheckDependencies()
{
    PG_ASSERT( !createInfo.filename.empty() );

    if ( outputFile.empty() )
    {
        outputFile = GetModelFastFileName( createInfo );
    }
    if ( !std::filesystem::exists( outputFile ) ||
         Timestamp( outputFile ) < Timestamp( createInfo.filename ) )
    {
        return ASSET_OUT_OF_DATE;
    }

    Timestamp outTimestamp( outputFile );

    return ASSET_UP_TO_DATE;
}

ConverterStatus ModelConverter::Convert()
{
    if ( status == ASSET_UP_TO_DATE )
    {
        return CONVERT_SUCCESS;
    }

    Model model;

    if ( !model.Load(&createInfo) )
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
