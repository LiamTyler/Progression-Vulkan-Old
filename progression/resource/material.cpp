#include "resource/material.hpp"
#include "core/assert.hpp"
#include "core/core_defines.hpp"
#include "resource/resource_manager.hpp"
#include "resource/resource_version_numbers.hpp"
#include "utils/fileIO.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"

namespace Progression
{

bool Material::Load( ResourceCreateInfo* createInfo )
{
    PG_ASSERT( createInfo );
    MaterialCreateInfo* info = static_cast< MaterialCreateInfo* >( createInfo );
    name     = info->name;
    Ka       = info->Ka;
    Kd       = info->Kd;
    Ks       = info->Ks;
    Ke       = info->Ke;
    Ns       = info->Ns;
    map_Kd   = ResourceManager::Get< Image >( info->map_Kd_name );
    map_Norm = ResourceManager::Get< Image >( info->map_Norm_name );

    return true;
}

void Material::Move( std::shared_ptr< Resource > dst )
{
    PG_ASSERT( std::dynamic_pointer_cast< Material >( dst ) );
    Material* dstPtr = (Material*) dst.get();
    *dstPtr          = std::move( *this );
}

bool Material::Serialize( std::ofstream& out ) const
{
    serialize::Write( out, name );
    serialize::Write( out, Ka );
    serialize::Write( out, Kd );
    serialize::Write( out, Ks );
    serialize::Write( out, Ke );
    serialize::Write( out, Ns );
    bool hasDiffuseTexture = map_Kd != nullptr;
    serialize::Write( out, hasDiffuseTexture );
    if ( hasDiffuseTexture )
    {
        bool texManaged = ResourceManager::Get< Image >( map_Kd->name ) != nullptr;
        serialize::Write( out, texManaged );
        if ( texManaged )
        {
            serialize::Write( out, map_Kd->name );
        }
        else
        {
            serialize::Write( out, (uint32_t) PG_RESOURCE_IMAGE_VERSION );
            serialize::Write( out, map_Kd->name );
            serialize::Write( out, map_Kd->GetImageFlags() );
            std::string samplerName = map_Kd->GetTexture()->GetSampler()->GetName();
            serialize::Write( out, samplerName );
            map_Kd->Serialize( out );
        }
    }

    bool hasNormalMap = map_Norm != nullptr;
    serialize::Write( out, hasNormalMap );
    if ( hasNormalMap )
    {
        bool texManaged = ResourceManager::Get< Image >( map_Norm->name ) != nullptr;
        serialize::Write( out, texManaged );
        if ( texManaged )
        {
            serialize::Write( out, map_Norm->name );
        }
        else
        {
            serialize::Write( out, (uint32_t) PG_RESOURCE_IMAGE_VERSION );
            serialize::Write( out, map_Norm->name );
            serialize::Write( out, map_Norm->GetImageFlags() );
            std::string samplerName = map_Norm->GetTexture()->GetSampler()->GetName();
            serialize::Write( out, samplerName );
            map_Norm->Serialize( out );
        }
    }

    return !out.fail();
}

bool Material::Deserialize( char*& buffer )
{
    serialize::Read( buffer, name );
    serialize::Read( buffer, Ka );
    serialize::Read( buffer, Kd );
    serialize::Read( buffer, Ks );
    serialize::Read( buffer, Ke );
    serialize::Read( buffer, Ns );
    bool hasDiffuseTexture;
    serialize::Read( buffer, hasDiffuseTexture );
    if ( hasDiffuseTexture )
    {
        bool texManaged;
        serialize::Read( buffer, texManaged );
        if ( texManaged )
        {
            std::string map_name;
            serialize::Read( buffer, map_name );
            map_Kd = ResourceManager::Get< Image >( map_name );
            PG_ASSERT( map_Kd, "No diffuse texture with name '" + map_name + "' found" );
        }
        else
        {
            uint32_t imVersion;
            serialize::Read( buffer, imVersion );
            PG_ASSERT( imVersion == PG_RESOURCE_IMAGE_VERSION, "Expected image version: " +
                std::to_string( PG_RESOURCE_IMAGE_VERSION ) + ", but got: " + std::to_string( imVersion ) );
            map_Kd = std::make_shared< Image >();
            map_Kd->Deserialize( buffer );
        }
    }

    bool hasNormalMap;
    serialize::Read( buffer, hasNormalMap );
    if ( hasNormalMap )
    {
        bool texManaged;
        serialize::Read( buffer, texManaged );
        if ( texManaged )
        {
            std::string map_name;
            serialize::Read( buffer, map_name );
            map_Norm = ResourceManager::Get< Image >( map_name );
            PG_ASSERT( map_Norm, "No normal map with name '" + map_name + "' found" );
        }
        else
        {
            uint32_t imVersion;
            serialize::Read( buffer, imVersion );
            PG_ASSERT( imVersion == PG_RESOURCE_IMAGE_VERSION, "Expected image version: " +
                std::to_string( PG_RESOURCE_IMAGE_VERSION ) + ", but got: " + std::to_string( imVersion ) );
            map_Norm = std::make_shared< Image >();
            map_Norm->Deserialize( buffer );
        }
    }

    return true;
}

bool Material::LoadMtlFile( std::vector< Material >& materials, const std::string& fname )
{
    std::ifstream file( fname );
    if ( !file )
    {
        LOG_ERR( "Could not open mtl file: ", fname );
        return false;
    }

    materials.clear();
    Material* mat = nullptr;

    std::string line;
    std::string first;
    while ( std::getline( file, line ) )
    {
        std::istringstream ss( line );
        ss >> first;
        if ( first == "#" )
        {
            continue;
        }
        else if ( first == "newmtl" )
        {
            std::string name;
            ss >> name;
            materials.emplace_back();
            mat = &materials[materials.size() - 1];
            mat->name = name;
        }
        else if ( first == "Ns" )
        {
            ss >> mat->Ns;
        }
        else if ( first == "Ka" )
        {
            ss >> mat->Ka;
        }
        else if ( first == "Kd" )
        {
            ss >> mat->Kd;
        }
        else if ( first == "Ks" )
        {
            ss >> mat->Ks;
        }
        else if ( first == "Ke" )
        {
            ss >> mat->Ke;
        }
        else if ( first == "map_Kd" )
        {
            std::string texName;
            ss >> texName;
            mat->map_Kd = ResourceManager::Get< Image >( texName );
            if ( !mat->map_Kd )
            {
                // LOG_ERR("Failed to load map_Kd image '", texName, "' in mtl file '", fname, "'");
                // return false;
                mat->map_Kd = Image::Load2DImageWithDefaultSettings( PG_RESOURCE_DIR + texName );
                if ( !mat->map_Kd )
                {
                    LOG_ERR( "Failed to load texture with default settings while parsing MTL file." );
                    return false;
                }
            }
        }
    }

    return true;
}

} // namespace Progression
