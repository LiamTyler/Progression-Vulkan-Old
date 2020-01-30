#include "resource/material.hpp"
#include "core/assert.hpp"
#include "core/core_defines.hpp"
#include "resource/resource_manager.hpp"
#include "resource/resource_version_numbers.hpp"
#include "utils/fileIO.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#include "utils/string.hpp"
#include <cctype>

namespace Progression
{

bool Material::Load( ResourceCreateInfo* createInfo )
{
    PG_ASSERT( createInfo );
    MaterialCreateInfo* info = static_cast< MaterialCreateInfo* >( createInfo );
    name        = info->name;
    Kd          = info->Kd;
    roughness   = info->roughness;
    metallic    = info->metallic;
    transparent = info->transparent;
    map_Kd      = ResourceManager::Get< Image >( info->map_Kd_name );
    map_Norm    = ResourceManager::Get< Image >( info->map_Norm_name );
    map_Pm      = ResourceManager::Get< Image >( info->map_Pm_name );
    map_Pr      = ResourceManager::Get< Image >( info->map_Pr_name );

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
    serialize::Write( out, Kd );
    serialize::Write( out, roughness );
    serialize::Write( out, metallic );
    serialize::Write( out, transparent );
    bool hasDiffuseTexture = map_Kd != nullptr;
    serialize::Write( out, hasDiffuseTexture );
    if ( hasDiffuseTexture )
    {
        serialize::Write( out, map_Kd->name );
    }

    bool hasNormalMap = map_Norm != nullptr;
    serialize::Write( out, hasNormalMap );
    if ( hasNormalMap )
    {
        serialize::Write( out, map_Norm->name );
    }

    bool hasMetatllicMap = map_Pm != nullptr;
    serialize::Write( out, hasMetatllicMap );
    if ( hasMetatllicMap )
    {
        serialize::Write( out, map_Pm->name );
    }

    bool hasRoughnessMap = map_Pr != nullptr;
    serialize::Write( out, hasRoughnessMap );
    if ( hasRoughnessMap )
    {
        serialize::Write( out, map_Pr->name );
    }

    return !out.fail();
}

bool Material::Deserialize( char*& buffer )
{
    serialize::Read( buffer, name );
    serialize::Read( buffer, Kd );
    serialize::Read( buffer, roughness );
    serialize::Read( buffer, metallic );
    serialize::Read( buffer, transparent );
    bool hasDiffuseTexture;
    serialize::Read( buffer, hasDiffuseTexture );
    if ( hasDiffuseTexture )
    {
        std::string map_name;
        serialize::Read( buffer, map_name );
        map_Kd = ResourceManager::Get< Image >( map_name );
        PG_ASSERT( map_Kd, "No diffuse texture with name '" + map_name + "' found" );
    }

    bool hasNormalMap;
    serialize::Read( buffer, hasNormalMap );
    if ( hasNormalMap )
    {
        std::string map_name;
        serialize::Read( buffer, map_name );
        map_Norm = ResourceManager::Get< Image >( map_name );
        PG_ASSERT( map_Norm, "No normal map with name '" + map_name + "' found" );
    }

    bool hasMetallicMap;
    serialize::Read( buffer, hasMetallicMap );
    if ( hasMetallicMap )
    {
        std::string map_name;
        serialize::Read( buffer, map_name );
        map_Pm = ResourceManager::Get< Image >( map_name );
        PG_ASSERT( map_Pm, "No metallic map with name '" + map_name + "' found" );
    }

    bool hasRoughnessMap;
    serialize::Read( buffer, hasRoughnessMap );
    if ( hasRoughnessMap )
    {
        std::string map_name;
        serialize::Read( buffer, map_name );
        map_Pr = ResourceManager::Get< Image >( map_name );
        PG_ASSERT( map_Pr, "No roughness map with name '" + map_name + "' found" );
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
        if ( IsWhiteSpace( line ) )
        {
            continue;
        }

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
        else if ( first == "Kd" )
        {
            ss >> mat->Kd;
        }
        else if ( first == "d" )
        {
            float d;
            ss >> d;
            if ( d == 1.0f )
            {
                mat->transparent = true;
            }
        }
        else if ( first == "Pr" )
        {
            ss >> mat->roughness;
        }
        else if ( first == "Pm" )
        {
            ss >> mat->metallic;
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
                    LOG_ERR( "Failed to load diffuse texture with default settings while parsing MTL file." );
                    return false;
                }
            }
        }
        else if ( first == "map_bump" || first == "norm" )
        {
            std::string texName;
            ss >> texName;
            mat->map_Norm = ResourceManager::Get< Image >( texName );
            if ( !mat->map_Norm )
            {
                mat->map_Norm = Image::Load2DImageWithDefaultSettings( PG_RESOURCE_DIR + texName, ImageSemantic::NORMAL );
                if ( !mat->map_Norm )
                {
                    LOG_ERR( "Failed to load normal map with default settings while parsing MTL file." );
                    return false;
                }
            }
        }
        else if ( first == "map_Pm" )
        {
            std::string texName;
            ss >> texName;
            mat->map_Pm = ResourceManager::Get< Image >( texName );
            if ( !mat->map_Pm )
            {
                mat->map_Pm = Image::Load2DImageWithDefaultSettings( PG_RESOURCE_DIR + texName, ImageSemantic::METALLIC );
                if ( !mat->map_Pm )
                {
                    LOG_ERR( "Failed to load metallic map with default settings while parsing MTL file." );
                    return false;
                }
            }
        }
        else if ( first == "map_Pr" )
        {
            std::string texName;
            ss >> texName;
            mat->map_Pr = ResourceManager::Get< Image >( texName );
            if ( !mat->map_Pr )
            {
                mat->map_Pr = Image::Load2DImageWithDefaultSettings( PG_RESOURCE_DIR + texName, ImageSemantic::ROUGHNESS );
                if ( !mat->map_Pr )
                {
                    LOG_ERR( "Failed to load roughness map with default settings while parsing MTL file." );
                    return false;
                }
            }
        }
    }

    return true;
}

} // namespace Progression
