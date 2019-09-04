#include "core/feature_defines.hpp"
#include "core/time.hpp"
#include "core/window.hpp"
#include "lz4/lz4.h"
#include "memory_map/MemoryMapped.h"
#include "resource/material.hpp"
#include "resource/model.hpp"
#include "resource/resource_manager.hpp"
#include "resource/shader.hpp"
#include "resource/texture.hpp"
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
        PG_MAYBE_UNUSED( start );

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

        char* data = uncompressedBuffer;
        LOG( "LZ4 decompress finished in: ", Time::GetDuration( start ), " ms." );
#endif // #if USING( LZ4_COMPRESSED_FASTFILES )

        {
            uint32_t numShaders;
            serialize::Read( data, numShaders );
            ResourceMap& resources = f_resources[GetResourceTypeID< Shader >()];
            for ( uint32_t i = 0; i < numShaders; ++i )
            {
                auto shader = std::make_shared< Shader >();
                if ( !shader->Deserialize( data ) )
                {
                    LOG_ERR( "Failed to load shader from fastfile" );
                    return false;
                }
                if ( resources.find( shader->name ) != resources.end() )
                {
                    shader->Move( resources[shader->name] );
                }
                else
                {
                    resources[shader->name] = shader;
                }
            }
        }

        {
            uint32_t numTextures;
            serialize::Read( data, numTextures );
            ResourceMap& resources = f_resources[GetResourceTypeID< Texture >()];
            for ( uint32_t i = 0; i < numTextures; ++i )
            {
                auto res = std::make_shared< Texture >();
                if ( !res->Deserialize( data ) )
                {
                    LOG_ERR( "Failed to load texture from fastfile" );
                    return false;
                }
                if ( resources.find( res->name ) != resources.end() )
                {
                    res->Move( resources[res->name] );
                }
                else
                {
                    resources[res->name] = res;
                }
            }
        }

        {
            uint32_t numMaterials;
            serialize::Read( data, numMaterials );
            ResourceMap& resources = f_resources[GetResourceTypeID< Material >()];
            for ( uint32_t i = 0; i < numMaterials; ++i )
            {
                auto res = std::make_shared< Material >();
                if ( !res->Deserialize( data ) )
                {
                    LOG_ERR( "Failed to load material from fastfile" );
                    return false;
                }
                if ( resources.find( res->name ) != resources.end() )
                {
                    res->Move( resources[res->name] );
                }
                else
                {
                    resources[res->name] = res;
                }
            }
        }

        {
            uint32_t numModels;
            serialize::Read( data, numModels );
            ResourceMap& resources = f_resources[GetResourceTypeID< Model >()];
            for ( uint32_t i = 0; i < numModels; ++i )
            {
                auto res = std::make_shared< Model >();
                if ( !res->Deserialize( data ) )
                {
                    LOG_ERR( "Failed to load model from fastfile" );
                    return false;
                }
                if ( resources.find( res->name ) != resources.end() )
                {
                    res->Move( resources[res->name] );
                }
                else
                {
                    resources[res->name] = res;
                }
            }
        }

        // free( uncompressedBuffer );

        LOG( "Loaded fastfile '", fname, "' in: ", Time::GetDuration( start ), " ms." );

        return true;
    }

    std::shared_ptr< Texture > GetOrCreateTexture( const std::string& texname )
    {
        if ( texname.empty() )
        {
            return nullptr;
        }

        auto tex = ResourceManager::Get< Texture >( texname );
        if ( tex )
        {
            return tex;
        }

        TextureCreateInfo info;
        info.name = texname;
        info.filename = texname;

        return Load< Texture >( &info );
    }

} // namespace ResourceManager
} // namespace Progression
