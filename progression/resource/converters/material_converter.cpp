#include "resource/converters/material_converter.hpp"
#include "resource/material.hpp"
#include "utils/serialize.hpp"
#include "utils/timestamp.hpp"
#include <filesystem>
#include <fstream>

using namespace Progression;

static std::string GetMTLFileFastFileName( const std::string& mtlFileName )
{
    namespace fs = std::filesystem;

    std::string absPath = mtlFileName.empty() ? "" : fs::absolute( mtlFileName );

    size_t hash = std::hash< std::string >{}( absPath );
    std::string basename = fs::path( mtlFileName ).filename();
    return PG_RESOURCE_DIR "cache/materials/" + basename + "_" + std::to_string( hash ) + ".ffi";
}

AssetStatus MaterialConverter::CheckDependencies()
{
    if ( outputFile.empty() )
    {
        outputFile = GetMTLFileFastFileName( inputFile );
    }
    if ( !std::filesystem::exists( outputFile ) )
    {
        LOG( "OUT OF DATE: FFI File for MTLFile '", inputFile, "' does not exist, convert required" );
        return ASSET_OUT_OF_DATE;
    }

    Timestamp outTimestamp( outputFile );
    Timestamp newestFileTime( inputFile );

    m_status = outTimestamp <= newestFileTime ? ASSET_OUT_OF_DATE : ASSET_UP_TO_DATE;

    if ( m_status == ASSET_OUT_OF_DATE )
    {
        LOG( "OUT OF DATE: MTLFile file'", inputFile, "' has newer timestamp than saved FFI" );
    }
    else
    {
        LOG( "UP TO DATE: MTLFile '", inputFile, "'" );
    }

    return m_status;
}

ConverterStatus MaterialConverter::Convert()
{
    if ( !force && m_status == ASSET_UP_TO_DATE )
    {
        return CONVERT_SUCCESS;
    }
    
    std::vector< Progression::Material > materials;

    if (! Progression::Material::LoadMtlFile( materials, inputFile ) )
    {
        LOG_ERR( "Failed to load MTLFile '", inputFile, "'" );
        return CONVERT_ERROR;
    }

    std::ofstream out( outputFile, std::ios::binary );

    for ( const auto& material : materials )
    {
        if (! material.Serialize( out ) )
        {
            LOG_ERR( "Could not save materials to FFI file: '", outputFile, "'" );
            return CONVERT_ERROR;
        }
    }

    out.close();

    return CONVERT_SUCCESS;
}
