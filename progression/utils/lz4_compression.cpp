#include "utils/lz4_compression.hpp"
#include "core/assert.hpp"
#include "lz4/lz4.h"
#include "memory_map/MemoryMapped.h"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"

namespace Progression
{

    bool LZ4CompressFile( const std::string& inputFilename, const std::string& outputFilename )
    {
        MemoryMapped memMappedFile;
        if ( !memMappedFile.open( inputFilename, MemoryMapped::WholeFile, MemoryMapped::Normal ) )
        {
            LOG_ERR( "Could not open file:", inputFilename );
            return false;
        }

        char* src = (char*) memMappedFile.getData();
        const int srcSize = (int) memMappedFile.size();

        const int maxDstSize = LZ4_compressBound( srcSize );

        char* compressedData = (char*) malloc( maxDstSize );
        const int compressedDataSize = LZ4_compress_default( src, compressedData, srcSize, maxDstSize );

        memMappedFile.close();

        if ( compressedDataSize <= 0 )
        {
            LOG_ERR( "Error while trying to compress the file. LZ4 returned: ", compressedDataSize );
            return false;
        }

        if ( compressedDataSize > 0 )
        {
            LOG( "Compressed file size ratio: ", (float) compressedDataSize / srcSize );
        }

        std::ofstream out( outputFilename, std::ios::binary );
        if ( !out )
        {
            LOG_ERR( "Failed to open file '", outputFilename, "' for writing compressed results" );
            return false;
        }

        serialize::Write( out, srcSize );
        serialize::Write( out, compressedData, compressedDataSize );

        return true;
    }

    char* LZ4DecompressMappedFile( char* compressedData, int fileSize )
    {
        int uncompressedSize;
        serialize::Read( compressedData, uncompressedSize );

        const int compressedSize   = fileSize - 4;
        char* uncompressedBuffer   = (char*) malloc( uncompressedSize );
        const int decompressedSize = LZ4_decompress_safe( compressedData, uncompressedBuffer, compressedSize, uncompressedSize );

        PG_ASSERT( decompressedSize >= 0, "Failed to decompress file with return value: " + std::to_string( decompressedSize ) );
        PG_ASSERT( decompressedSize == uncompressedSize, "Decompressed data size does not match the expected uncompressed size" );

        return uncompressedBuffer;
    }

} // namespace Progression