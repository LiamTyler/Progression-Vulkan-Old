#include "resource/material.hpp"
#include "resource/resource_manager.hpp"
#include "resource/texture.hpp"
#include "utils/fileIO.hpp"
#include "utils/serialize.hpp"

namespace Progression
{

bool Material::Load( ResourceCreateInfo* createInfo )
{
    PG_ASSERT( createInfo );
    MaterialCreateInfo* info = static_cast< MaterialCreateInfo* >( createInfo );
    name = info->name;
    Ka = info->Ka;
    Kd = info->Kd;
    Ks = info->Ks;
    Ke = info->Ke;
    Ns = info->Ns;
    map_Kd = ResourceManager::GetOrCreateTexture( info->map_Kd_name );

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
    std::string map_Kd_name = map_Kd ? map_Kd->name : "";
    serialize::Write( out, map_Kd_name );

    return !out.fail();
}

bool Material::Deserialize( std::ifstream& in )
{
    serialize::Read( in, name );
    serialize::Read( in, Ka );
    serialize::Read( in, Kd );
    serialize::Read( in, Ks );
    serialize::Read( in, Ke );
    serialize::Read( in, Ns );
    std::string map_Kd_name = map_Kd ? map_Kd->name : "";
    serialize::Read( in, map_Kd_name );
    if ( !map_Kd_name.empty() )
    {
        map_Kd = ResourceManager::Get< Texture >( map_Kd_name );
        PG_ASSERT( map_Kd );
    }

    return !in.fail();
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
            // mat->map_Kd = ResourceManager::GetOrCreateTexture( rootTexDir + texName );
            // if ( !mat->map_Kd )
            // {
            //     LOG_ERR("Failed to load/create map_Kd texture '", texName, "' in mtl file '", fname, "'");
            //     return false;
            // }
            mat->map_Kd = ResourceManager::Get< Texture >( texName );
            if ( !mat->map_Kd )
            {
                LOG_ERR("Failed to load map_Kd texture '", texName, "' in mtl file '", fname, "'");
                return false;
            }
        }
    }

    return true;
}

} // namespace Progression
