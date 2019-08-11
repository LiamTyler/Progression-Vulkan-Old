#include "resource/resource_manager.hpp"
#include "core/configuration.hpp"
#include "core/time.hpp"
#include "core/window.hpp"
#include "memory_map/MemoryMapped.h"
#include "resource/material.hpp"
#include "resource/model.hpp"
#include "resource/shader.hpp"
#include "resource/texture.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#include <future>
#include <mutex>
#include <sys/stat.h>
#include <thread>
#include <tuple>

namespace Progression
{

namespace ResourceManager
{

    ResourceDB f_resources;

    void Init()
    {
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
        std::ifstream in( fname, std::ios::binary );
        if ( !in )
        {
            LOG_ERR( "Could not open fastfile:", fname );
            return false;
        }

        auto start = Time::GetTimePoint();

        {
            uint32_t numShaders;
            serialize::Read( in, numShaders );
            ResourceMap& resources = f_resources[GetResourceTypeID< Shader >()];
            for ( uint32_t i = 0; i < numShaders; ++i )
            {
                auto shader = std::make_shared< Shader >();
                if ( !shader->Deserialize( in ) )
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
            serialize::Read( in, numTextures );
            ResourceMap& resources = f_resources[GetResourceTypeID< Texture >()];
            for ( uint32_t i = 0; i < numTextures; ++i )
            {
                auto res = std::make_shared< Texture >();
                if ( !res->Deserialize( in ) )
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
            serialize::Read( in, numMaterials );
            ResourceMap& resources = f_resources[GetResourceTypeID< Material >()];
            for ( uint32_t i = 0; i < numMaterials; ++i )
            {
                auto res = std::make_shared< Material >();
                if ( !res->Deserialize( in ) )
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
            serialize::Read( in, numModels );
            ResourceMap& resources = f_resources[GetResourceTypeID< Model >()];
            for ( uint32_t i = 0; i < numModels; ++i )
            {
                auto res = std::make_shared< Model >();
                if ( !res->Deserialize( in ) )
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

        in.close();

        LOG( "Loaded fastfile '", fname, "' in: ", Time::GetDuration( start ), " ms." );

        return true;
    }

    bool LoadFastFile2( const std::string& fname )
    {
        MemoryMapped memMappedFile;
        if ( !memMappedFile.open( fname, MemoryMapped::WholeFile, MemoryMapped::SequentialScan ) )
        {
            LOG_ERR( "Could not open fastfile:", fname );
            return false;
        }

        auto start = Time::GetTimePoint();

        char* data = (char*) memMappedFile.getData();
        {
            uint32_t numShaders;
            serialize::Read( data, numShaders );
            ResourceMap& resources = f_resources[GetResourceTypeID< Shader >()];
            for ( uint32_t i = 0; i < numShaders; ++i )
            {
                auto shader = std::make_shared< Shader >();
                if ( !shader->Deserialize2( data ) )
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
                if ( !res->Deserialize2( data ) )
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
                if ( !res->Deserialize2( data ) )
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
                if ( !res->Deserialize2( data ) )
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

        memMappedFile.close();

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
