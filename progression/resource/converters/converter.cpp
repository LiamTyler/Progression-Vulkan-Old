#include "resource/converters/converter.hpp"
#include "memory_map/MemoryMapped.h"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"

bool Converter::WriteToFastFile( std::ofstream& out ) const
{
    MemoryMapped memMappedFile;
    if ( !memMappedFile.open( m_outputSettingsFile, MemoryMapped::WholeFile, MemoryMapped::SequentialScan ) )
    {
        LOG_ERR( "Error opening intermediate file '", m_outputSettingsFile, "'" );
        return false;
    }
    serialize::Write( out, (char*) memMappedFile.getData(), memMappedFile.size() );

    memMappedFile.close();

    if ( !memMappedFile.open( m_outputContentFile, MemoryMapped::WholeFile, MemoryMapped::SequentialScan ) )
    {
        LOG_ERR( "Error opening intermediate file '", m_outputContentFile, "'" );
        return false;
    }

    serialize::Write( out, (char*) memMappedFile.getData(), memMappedFile.size() );

    return true;
}

std::string Converter::GetName() const
{
    return m_outputContentFile;
}

AssetStatus Converter::GetStatus() const
{
    return m_status;
}