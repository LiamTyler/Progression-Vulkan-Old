#pragma once

#include <string>

namespace Progression
{

    bool LZ4CompressFile( const std::string& inputFilename, const std::string& outputFilename );

    char* LZ4DecompressMappedFile( char* compressedData, int fileSize );

} // namespace Progression