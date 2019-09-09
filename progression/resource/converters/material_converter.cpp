#include "resource/converters/material_converter.hpp"
#include "memory_map/MemoryMapped.h"
#include "resource/material.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#include "utils/timestamp.hpp"
#include <filesystem>
#include <fstream>

using namespace Progression;

static std::string GetContentFastFileName( const std::string& mtlFileName )
{
    namespace fs = std::filesystem;

    std::string absPath = mtlFileName.empty() ? "" : fs::absolute( mtlFileName ).string();

    size_t hash = std::hash< std::string >{}( absPath );
    std::string basename = fs::path( mtlFileName ).filename().string();
    return PG_RESOURCE_DIR "cache/materials/" + basename + "_" + std::to_string( hash ) + ".ffi";
}

AssetStatus MaterialConverter::CheckDependencies()
{
    m_outputContentFile = GetContentFastFileName( inputFile );
    if ( !std::filesystem::exists( m_outputContentFile ) )
    {
        LOG( "OUT OF DATE: FFI File for MTLFile '", inputFile, "' does not exist, convert required" );
        return ASSET_OUT_OF_DATE;
    }

    Timestamp outTimestamp( m_outputContentFile );
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
    if ( m_status == ASSET_UP_TO_DATE )
    {
        return CONVERT_SUCCESS;
    }
    
    std::vector< Progression::Material > materials;

    if ( !Progression::Material::LoadMtlFile( materials, inputFile ) )
    {
        LOG_ERR( "Failed to load MTLFile '", inputFile, "'" );
        return CONVERT_ERROR;
    }

    std::ofstream out( m_outputContentFile, std::ios::binary );

    for ( const auto& material : materials )
    {
        if (! material.Serialize( out ) )
        {
            LOG_ERR( "Could not save materials to FFI file: '", m_outputContentFile, "'" );
            return CONVERT_ERROR;
        }
    }

    out.close();

    return CONVERT_SUCCESS;
}

bool MaterialConverter::WriteToFastFile( std::ofstream& out ) const
{
    MemoryMapped memMappedFile;
    if ( !memMappedFile.open( m_outputContentFile, MemoryMapped::WholeFile, MemoryMapped::SequentialScan ) )
    {
        LOG_ERR( "Error opening intermediate file '", m_outputContentFile, "'" );
        return false;
    }

    serialize::Write( out, (char*) memMappedFile.getData(), memMappedFile.size() );

    return true;
}
