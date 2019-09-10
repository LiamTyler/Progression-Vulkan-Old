#include "core/feature_defines.hpp"
#include "core/time.hpp"
#include "core/window.hpp"
#include "lz4/lz4.h"
#include "memory_map/MemoryMapped.h"
#include "resource/image.hpp"
#include "resource/material.hpp"
#include "resource/model.hpp"
#include "resource/resource_manager.hpp"
#include "resource/shader.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"

uint32_t BaseFamily::typeCounter_ = 0;

namespace Progression
{

namespace ResourceManager
{

    ResourceDB f_resources;

    void Init()
    {
        f_resources.Clear();
        // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        auto defaultMat                                         = std::make_shared< Material >();
        defaultMat->Kd                                          = glm::vec3( 1, 1, 0 );
        f_resources[GetResourceTypeID< Material >()]["default"] = defaultMat;
    }

    void Shutdown()
    {
        f_resources.Clear();
    }

    bool LoadFastFile( const std::string& fname )
    {
        MemoryMapped memMappedFile;
        if ( !memMappedFile.open( fname, MemoryMapped::WholeFile, MemoryMapped::SequentialScan ) )
        {
            LOG_ERR( "Could not open fastfile:", fname );
            return false;
        }

        auto start = Time::GetTimePoint();

        char* data = (char*) memMappedFile.getData();
#if USING( LZ4_COMPRESSED_FASTFILES )
        char* fileData = (char*) memMappedFile.getData();
        int uncompressedSize;
        serialize::Read( fileData, uncompressedSize );

        const int compressedSize = static_cast<int>( memMappedFile.size() - 4 );
        char* uncompressedBuffer = (char*) malloc( uncompressedSize );
        const int decompressedSize = LZ4_decompress_safe( fileData, uncompressedBuffer, compressedSize, uncompressedSize );

        memMappedFile.close();

        if ( decompressedSize < 0 )
        {
            LOG_ERR( "Failed to decompress fastfile with return value: ", decompressedSize );
            return false;
        }

        if ( decompressedSize != uncompressedSize )
        {
            LOG_ERR( "Decompressed data size does not match the expected uncompressed size: ",
                     decompressedSize, " != ", uncompressedSize );
        }

        data = uncompressedBuffer;
        LOG( "LZ4 decompress finished in: ", Time::GetDuration( start ), " ms." );
#endif // #if USING( LZ4_COMPRESSED_FASTFILES )

#define LOAD_RESOURCES( type ) \
        { \
            uint32_t numRes; \
            serialize::Read( data, numRes ); \
            ResourceMap& resources = f_resources[GetResourceTypeID< type >()]; \
            for ( uint32_t i = 0; i < numRes; ++i ) \
            { \
                auto res = std::make_shared< type >(); \
                if ( !res->Deserialize( data ) ) \
                { \
                    LOG_ERR( "Failed to load type from fastfile" ); \
                    return false; \
                } \
                if ( resources.find( res->name ) != resources.end() ) \
                { \
                    res->Move( resources[res->name] ); \
                } \
                else \
                { \
                    resources[res->name] = res; \
                } \
            } \
        } \


        LOAD_RESOURCES( Shader );
        LOAD_RESOURCES( Image );
        LOAD_RESOURCES( Material );
        LOAD_RESOURCES( Model );

        LOG( "Loaded fastfile '", fname, "' in: ", Time::GetDuration( start ), " ms." );

        return true;
    }

} // namespace ResourceManager
} // namespace Progression
