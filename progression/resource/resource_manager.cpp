#include "core/feature_defines.hpp"
#include "core/assert.hpp"
#include "core/time.hpp"
#include "core/window.hpp"
#include "lz4/lz4.h"
#include "memory_map/MemoryMapped.h"
#include "resource/image.hpp"
#include "resource/material.hpp"
#include "resource/model.hpp"
#include "resource/resource_manager.hpp"
#include "resource/resource_version_numbers.hpp"
#include "resource/script.hpp"
#include "resource/shader.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#include "utils/type_name.hpp"

uint32_t BaseFamily::typeCounter_ = 0;

#define AUTORUN_CONVERTER_ON_FF_LOAD IN_USE

namespace Progression
{

namespace ResourceManager
{

    ResourceDB f_resources = {};

    void Init()
    {
        f_resources.Clear();
        auto defaultMat                                         = std::make_shared< Material >();
        defaultMat->Kd                                          = glm::vec3( 1, 1, 0 );
        f_resources[GetResourceTypeID< Material >()]["default"] = defaultMat;
    }

    void Shutdown()
    {
        f_resources.Clear();
    }

    template< typename ResourceType >
    static bool DeserializeResources( char*& data, uint32_t expectedVersion )
    {
        uint32_t numRes;
        serialize::Read( data, numRes );
        if constexpr ( !std::is_same< ResourceType, Material >::value )
        {
            uint32_t version;
            serialize::Read( data, version );
            PG_ASSERT( version == expectedVersion, std::string( "Resource type '" ) + type_name< ResourceType >().data() +
                "', expected version: " + std::to_string( expectedVersion ) + ", but found: " + std::to_string( version ) );
        }
        ResourceMap& resources = f_resources[GetResourceTypeID< ResourceType >()];
        for ( uint32_t i = 0; i < numRes; ++i )
        {
            auto res = std::make_shared< ResourceType >();
            if ( !res->Deserialize( data ) )
            {
                LOG_ERR( "Failed to load type from fastfile" );
                return false;
            }
            if ( resources.find( res->name ) != resources.end() )
            {
                LOG_WARN( "Resource of type '", type_name< ResourceType >(), "' and name '", res->name, "' is already in resource manager, overwritting" );
                res->Move( resources[res->name] );
            }
            else
            {
                resources[res->name] = res;
            }
        }
        if constexpr ( !std::is_same< ResourceType, Material >::value )
        {
            size_t magicNumberGuard;
            serialize::Read( data, magicNumberGuard );
            PG_ASSERT( magicNumberGuard == PG_RESOURCE_MAGIC_NUMBER_GUARD, "serializatio and deserialization do not match" );
        }

        return true;
    }

    static char* DecompressFF( char* compressedData, int fileSize )
    {
        auto start = Time::GetTimePoint();
        int uncompressedSize;
        serialize::Read( compressedData, uncompressedSize );

        const int compressedSize   = fileSize - 4;
        char* uncompressedBuffer   = (char*) malloc( uncompressedSize );
        const int decompressedSize = LZ4_decompress_safe( compressedData, uncompressedBuffer, compressedSize, uncompressedSize );

        PG_ASSERT( decompressedSize >= 0, "Failed to decompress fastfile with return value: " + std::to_string( decompressedSize ) );
        PG_ASSERT( decompressedSize == uncompressedSize, "Decompressed data size does not match the expected uncompressed size" );

        LOG( "LZ4 decompress finished in: ", Time::GetDuration( start ), " ms." );
        return uncompressedBuffer;
    }

    bool LoadFastFile( const std::string& fname, bool runConverterIfEnabled )
    {
        MemoryMapped memMappedFile;
        if ( !memMappedFile.open( fname, MemoryMapped::WholeFile, MemoryMapped::SequentialScan ) )
        {
            LOG_ERR( "Could not open fastfile: '", fname, "'" );
            return false;
        }

        auto start = Time::GetTimePoint();

        char* data = (char*) memMappedFile.getData();
#if USING( LZ4_COMPRESSED_FASTFILES )
        data = DecompressFF( (char*) memMappedFile.getData(), memMappedFile.size() - 4 );
        char* uncompressedStartPtr = data;
        memMappedFile.close();
#endif // #if USING( LZ4_COMPRESSED_FASTFILES )

        std::string originalFile;
        serialize::Read( data, originalFile );

        if ( runConverterIfEnabled )
        {
        #if USING( AUTORUN_CONVERTER_ON_FF_LOAD )
            memMappedFile.close();
        #if USING( LZ4_COMPRESSED_FASTFILES )
            free( uncompressedStartPtr );
        #endif // #if USING( LZ4_COMPRESSED_FASTFILES )

            std::string command = "\"\"" PG_BIN_DIR;
        #if USING( RELEASE_BUILD )
            command += "converter_release\"";
        #elif USING( SHIP_BUILD ) // #if USING( RELEASE_BUILD )
            command += "converter_ship\"";
        #else // #elif USING( SHIP_BUILD ) // #if USING( RELEASE_BUILD )
            command += "converter_debug\"";
        #endif // #else // #elif USING( SHIP_BUILD ) // #if USING( RELEASE_BUILD )

            command += " \"" + originalFile + "\"\"";
            LOG( command );
            int ret = system( command.c_str() );
            if ( ret != 0 )
            {
                LOG_ERR( "Error while running converter" );
                return false;
            }

            return LoadFastFile( fname, false );
        #endif // #if USING( AUTORUN_CONVERTER_ON_FF_LOAD )
        }

        bool success = true;
        success = success && DeserializeResources< Shader >( data, PG_RESOURCE_SHADER_VERSION );
        success = success && DeserializeResources< Image >( data, PG_RESOURCE_IMAGE_VERSION );
        if ( success )
        {
            uint32_t numMaterialConvs, version;
            serialize::Read( data, numMaterialConvs );
            serialize::Read( data, version );
            PG_ASSERT( version == PG_RESOURCE_MATERIAL_VERSION, std::string( "Resource type '" ) + type_name< Material >().data() +
                "', expected version: " + std::to_string( PG_RESOURCE_MATERIAL_VERSION ) + ", but found: " + std::to_string( version ) );
            for ( uint32_t matConv = 0; matConv < numMaterialConvs; ++matConv )
            {
                success = success && DeserializeResources< Material >( data, PG_RESOURCE_MATERIAL_VERSION );
            }
            if ( success )
            {
                size_t magicNumberGuard;
                serialize::Read( data, magicNumberGuard );
                PG_ASSERT( magicNumberGuard == PG_RESOURCE_MAGIC_NUMBER_GUARD, "serialization and deserialization do not match" );
            }
        }
        success = success && DeserializeResources< Model >( data, PG_RESOURCE_MODEL_VERSION );
        success = success && DeserializeResources< Script >( data, PG_RESOURCE_SCRIPT_VERSION );

        PG_MAYBE_UNUSED( start );
        LOG( "Loaded fastfile '", fname, "' in: ", Time::GetDuration( start ), " ms." );

#if USING( LZ4_COMPRESSED_FASTFILES )
        free( uncompressedStartPtr );
#endif // #if USING( LZ4_COMPRESSED_FASTFILES )

        return true;
    }

} // namespace ResourceManager
} // namespace Progression
