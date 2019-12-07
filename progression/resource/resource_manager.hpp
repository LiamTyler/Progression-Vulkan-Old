#pragma once

#include "resource/resource.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

class BaseFamily {
public:
    static uint32_t typeCounter_;
};

template <typename Derived>
class DerivedFamily : public BaseFamily {
public:
    static uint32_t typeCounter() {
        static uint32_t typeIndex = typeCounter_++;
        return typeIndex;
    }
};

template < typename Resource >
uint32_t GetResourceTypeID()
{
    return DerivedFamily< Resource >::typeCounter();
}

namespace Progression
{

enum ResourceTypes
{
    SHADER = 0,
    IMAGE,
    MATERIAL,
    MODEL,
    SCRIPT,
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
    bool LoadFastFile( const std::string& fname, bool runConverterIfEnabled = true );
    void Shutdown();

    template < typename T >
    std::shared_ptr< T > Get( const std::string& name )
    {
        static_assert( std::is_base_of< Resource, T >::value && !std::is_same< Resource, T >::value,
                       "Can only add resources to manager that inherit from class Resource" );
        auto& group = f_resources[GetResourceTypeID< T >()];
        auto it     = group.find( name );
        return it == group.end() ? nullptr : std::dynamic_pointer_cast< T >( it->second );
    }

    template < typename T >
    std::shared_ptr< T > LoadInternal( ResourceCreateInfo* createInfo )
    {
        static_assert( std::is_base_of< Resource, T >::value && !std::is_same< Resource, T >::value,
                       "Can only call load with a class that inherits from Resource" );

        auto currentResPtr = Get< T >( createInfo->name );
        if ( currentResPtr )
        {
            return currentResPtr;
        }

        auto resourcePtr  = std::make_shared< T >();
        resourcePtr->name = createInfo->name;
        if ( !resourcePtr->Load( createInfo ) )
        {
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

} // namespace ResourceManager
} // namespace Progression
