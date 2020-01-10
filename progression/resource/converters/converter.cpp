#include "resource/converters/converter.hpp"
#include "memory_map/MemoryMapped.h"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#include <filesystem>

bool Converter::WriteToFastFile( std::ofstream& out, bool debugMode ) const
{
    MemoryMapped memMappedFile;
    if ( !memMappedFile.open( m_outputSettingsFile, MemoryMapped::WholeFile, MemoryMapped::SequentialScan ) )
    {
        LOG_ERR( "Error opening intermediate file '", m_outputSettingsFile, "'" );
        return false;
    }
    serialize::Write( out, (char*) memMappedFile.getData(), memMappedFile.size() );

    memMappedFile.close();

    std::string fname = m_outputContentFile;
    if ( debugMode )
    {
        if ( std::filesystem::exists( fname + "d" ) )
        {
            fname += "d";
        }
    }
    if ( !memMappedFile.open( fname, MemoryMapped::WholeFile, MemoryMapped::SequentialScan ) )
    {
        LOG_ERR( "Error opening intermediate file '", fname, "'" );
        return false;
    }

    serialize::Write( out, (char*) memMappedFile.getData(), memMappedFile.size() );

    return true;
}

std::string Converter::GetName() const
{
    return m_outputContentFile;
}