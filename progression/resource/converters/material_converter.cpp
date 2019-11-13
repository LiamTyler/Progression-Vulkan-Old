#include "resource/converters/material_converter.hpp"
#include "memory_map/MemoryMapped.h"
#include "resource/material.hpp"
#include "resource/resource_version_numbers.hpp"
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

    std::string hash     = std::to_string( std::hash< std::string >{}( absPath ) );
    std::string basename = fs::path( mtlFileName ).filename().string();
    std::string version  = std::to_string( PG_RESOURCE_MATERIAL_VERSION );
    return PG_RESOURCE_DIR "cache/materials/" + basename + "_" + version + "_" + hash + ".ffi";
}

MaterialConverter::MaterialConverter( bool force_, bool verbose_ )
{
    force   = force_;
    verbose = verbose_;
}

AssetStatus MaterialConverter::CheckDependencies()
{
    m_outputContentFile = GetContentFastFileName( inputFile );
    IF_VERBOSE_MODE( LOG( "\nMTLFile '", inputFile, "' outputs FFI '", m_outputContentFile, "'" ) );
    if ( !std::filesystem::exists( m_outputContentFile ) )
    {
        LOG( "OUT OF DATE: FFI File for MTLFile '", inputFile, "' does not exist, convert required" );
        return ASSET_OUT_OF_DATE;
    }

    Timestamp outTimestamp( m_outputContentFile );
    Timestamp newestFileTime( inputFile );
    
    IF_VERBOSE_MODE( LOG( "MTLFile timestamp: ", newestFileTime, ", FFI timestamp: ", outTimestamp ) );

    status = outTimestamp <= newestFileTime ? ASSET_OUT_OF_DATE : ASSET_UP_TO_DATE;

    if ( status == ASSET_OUT_OF_DATE )
    {
        LOG( "OUT OF DATE: MTLFile file'", inputFile, "' has newer timestamp than saved FFI" );
    }
    else
    {
        if ( force )
        {
            LOG( "UP TO DATE: MTLFile '", inputFile, "', but --force used, so converting anyways\n" );
            status = ASSET_OUT_OF_DATE;
        }
        else
        {
            LOG( "UP TO DATE: MTLFile '", inputFile, "'" );
        }
    }

    return status;
}

ConverterStatus MaterialConverter::Convert()
{
    if ( status == ASSET_UP_TO_DATE )
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

    serialize::Write( out, static_cast< uint32_t >( materials.size() ) );
    for ( const auto& material : materials )
    {
        if ( !material.Serialize( out ) )
        {
            LOG_ERR( "Could not save materials to FFI file: '", m_outputContentFile, "'" );
            return CONVERT_ERROR;
        }
        ++numMaterials;
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

std::string MaterialConverter::GetName() const
{
    return inputFile;
}
