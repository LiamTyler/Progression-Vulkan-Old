#pragma once

#include "resource/resource.hpp"
#include "utils/logger.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

class ResourceTypeID
{
    inline static uint32_t identifier;

    template < typename... >
    inline static const auto inner = identifier++;

public:
    template < typename... Type >
    inline static const uint32_t id = inner< std::decay_t< Type >... >;
};

template < typename Resource >
uint32_t GetResourceTypeID()
{
    return ResourceTypeID::id< Resource >;
}

namespace Progression
{

class Texture;

enum ResourceTypes
{
    SHADER = 0,
    TEXTURE,
    MATERIAL,
    MODEL,
    TOTAL_RESOURCE_TYPES
};

namespace ResourceManager
{

    using ResourceMap = std::unordered_map< std::string, std::shared_ptr< Resource > >;

    class ResourceDB
    {
    public:
        ResourceDB()
        {
            maps.resize( TOTAL_RESOURCE_TYPES );
        }

        template < typename T >
        ResourceMap& GetMap()
        {
            return maps[GetResourceTypeID< T >()];
        }

        ResourceMap& operator[]( uint32_t typeID )
        {
            return maps[typeID];
        }

        void Clear()
        {
            maps.clear();
            maps.resize( TOTAL_RESOURCE_TYPES );
        }

        std::vector< ResourceMap > maps;
    };

    extern ResourceDB f_resources;

    void Init();
    bool LoadFastFile( const std::string& fname );
    void Shutdown();

    template < typename T >
    std::shared_ptr< T > Get( const std::string& name )
    {
        auto& group = f_resources[GetResourceTypeID< T >()];
        auto it     = group.find( name );
        return it == group.end() ? nullptr : std::static_pointer_cast< T >( it->second );
    }

    template < typename T >
    std::shared_ptr< T > LoadInternal( ResourceCreateInfo* createInfo )
    {
        static_assert( std::is_base_of< Resource, T >::value && !std::is_same< Resource, T >::value,
                       "Can only call load with a class that inherits from Resource" );
        // PG_ASSERT( createInfo != nullptr );

        auto currentResPtr = Get< T >( createInfo->name );
        if ( currentResPtr )
        {
            return currentResPtr;
        }

        auto resourcePtr  = std::make_shared< T >();
        resourcePtr->name = createInfo->name;
        if ( !resourcePtr->Load( createInfo ) )
        {
            LOG_ERR( "Failed to load resource with name '", createInfo->name, "'" );
            return nullptr;
        }
        f_resources.GetMap< T >()[createInfo->name] = resourcePtr;

        return resourcePtr;
    }

    template < typename T >
    std::shared_ptr< T > Load( ResourceCreateInfo* createInfo )
    {
        return LoadInternal< T >( createInfo );
    }

    template < typename T >
    void Add( std::shared_ptr< T > resourcePtr )
    {
        static_assert( std::is_base_of< Resource, T >::value && !std::is_same< Resource, T >::value,
                       "Can only add resources to manager that inherit from class Resource" );

        f_resources.GetMap< T >()[resourcePtr->name] = resourcePtr;         
    }

    std::shared_ptr< Texture > GetOrCreateTexture( const std::string& texname );

} // namespace ResourceManager
} // namespace Progression
