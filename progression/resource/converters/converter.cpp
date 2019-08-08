#include "resource/converters/converter.hpp"
#include "utils/serialize.hpp"

void ConvertHeader::serialize( std::ofstream& out )
{
    uint32_t numFiles = (uint32_t) fileDependencies.size();
    serialize::Write( out, numFiles );
    for ( const auto& file : fileDependencies )
    {
        serialize::Write( out, file );
    }
}

void ConvertHeader::deserialize( std::ifstream& in )
{
    uint32_t numFiles;
    serialize::Read( in, numFiles );
    for ( uint32_t i = 0; i < numFiles; ++i )
    {
        std::string tmp;
        serialize::Read( in, tmp );
        fileDependencies.push_back( tmp );
    }
}

AssetStatus Converter::GetStatus() const
{
    return m_status;
}