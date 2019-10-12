#include "resource/model.hpp"
#include "resource/material.hpp"
#include "resource/resource_manager.hpp"
#include "tinyobjloader/tiny_obj_loader.h"
#include "utils/fileIO.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#include <limits>

namespace Progression
{

Model::Model( Model&& model )
{
    *this = std::move( model );
}

Model& Model::operator=( Model&& model )
{
    name      = std::move( model.name );
    meshes    = std::move( model.meshes );
    materials = std::move( model.materials );

    return *this;
}

void Model::Move( std::shared_ptr< Resource > dst )
{
    PG_ASSERT( std::dynamic_pointer_cast< Model >( dst ) );
    Model* dstPtr = (Model*) dst.get();
    *dstPtr       = std::move( *this );
}

bool Model::Load( ResourceCreateInfo* createInfo )
{
    PG_ASSERT( createInfo );
    ModelCreateInfo* info = static_cast< ModelCreateInfo* >( createInfo );
    name = info->name;
    return LoadFromObj( info );
}

void Model::Free( bool gpuCopy, bool cpuCopy )
{
    for ( auto& mesh : meshes )
    {
        mesh.Free( gpuCopy, cpuCopy );
    }
}

bool Model::Serialize( std::ofstream& out ) const
{
    // serialize::Write( out, name );
    uint32_t numMeshes = static_cast< uint32_t >( meshes.size() );
    serialize::Write( out, numMeshes );
    for ( uint32_t i = 0; i < numMeshes; ++i )
    {
        if ( !materials[i]->Serialize( out ) )
        {
            LOG( "Could not write material: ", i, ", of model: ", name, " to fastfile" );
            return false;
        }
    }

    for ( uint32_t i = 0; i < numMeshes; ++i )
    {
        if ( !meshes[i].Serialize( out ) )
        {
            LOG( "Could not write mesh: ", i, ", of model: ", name, " to fastfile" );
            return false;
        }
    }

    serialize::Write( out, aabb.min );
    serialize::Write( out, aabb.max );
    serialize::Write( out, aabb.extent );

    return !out.fail();
}

bool Model::Deserialize( char*& buffer )
{
    serialize::Read( buffer, name );
    bool freeCpuCopy;
    bool createGpuCopy;
    serialize::Read( buffer, freeCpuCopy );
    serialize::Read( buffer, createGpuCopy );
    uint32_t numMeshes;
    serialize::Read( buffer, numMeshes );
    meshes.resize( numMeshes );
    materials.resize( numMeshes );
    for ( uint32_t i = 0; i < numMeshes; ++i )
    {
        auto mat = std::make_shared< Material >();
        if ( !mat->Deserialize( buffer ) )
        {
            LOG( "Could not load material: ", i, ", of model: ", name, " from fastfile" );
            return false;
        }
        materials[i] = mat;
    }

    for ( uint32_t i = 0; i < numMeshes; ++i )
    {
        if ( !meshes[i].Deserialize( buffer, createGpuCopy, freeCpuCopy ) )
        {
            LOG( "Could not load mesh: ", i, ", of model: ", name, " from fastfile" );
            return false;
        }
    }

    serialize::Read( buffer, aabb.min );
    serialize::Read( buffer, aabb.max );
    serialize::Read( buffer, aabb.extent );

    return true;
}

bool Model::LoadFromObj( ModelCreateInfo* createInfo )
{
    PG_ASSERT( createInfo->createGpuCopy || !createInfo->freeCpuCopy );
    tinyobj::attrib_t attrib;
    std::vector< tinyobj::shape_t > shapes;
    std::vector< tinyobj::material_t > tiny_materials;
    std::string err;
    bool ret = tinyobj::LoadObj( &attrib, &shapes, &tiny_materials, &err,
                                 createInfo->filename.c_str(), PG_RESOURCE_DIR, true );

    if ( !err.empty() )
    {
        LOG_WARN( "TinyObj loader warning: ", err );
    }

    if ( !ret )
    {
        LOG_ERR( "Failed to load the obj file: ", createInfo->filename );
        return false;
    }

    meshes.clear();
    materials.clear();

    aabb.min = glm::vec3( std::numeric_limits< float >::max() );
    aabb.max = glm::vec3( -std::numeric_limits< float >::max() );

    for ( int currentMaterialID = -1; currentMaterialID < (int) tiny_materials.size();
          ++currentMaterialID )
    {
        auto currentMaterial = std::make_shared< Material >();
        if ( currentMaterialID == -1 )
        {
            currentMaterial->name = "default";
        }
        else
        {
            tinyobj::material_t& mat = tiny_materials[currentMaterialID];
            currentMaterial->name    = mat.name;
            currentMaterial->Ka      = glm::vec3( mat.ambient[0], mat.ambient[1], mat.ambient[2] );
            currentMaterial->Kd      = glm::vec3( mat.diffuse[0], mat.diffuse[1], mat.diffuse[2] );
            currentMaterial->Ks      = glm::vec3( mat.specular[0], mat.specular[1], mat.specular[2] );
            currentMaterial->Ke      = glm::vec3( mat.emission[0], mat.emission[1], mat.emission[2] );
            currentMaterial->Ns      = mat.shininess;
            if ( !mat.diffuse_texname.empty() )
            {
                currentMaterial->map_Kd  = ResourceManager::Get< Image >( mat.diffuse_texname );
                if ( !currentMaterial->map_Kd )
                {
                    LOG_ERR( "Could not load material '", mat.name, "' from OBJ '", createInfo->filename, "'" );
                    LOG_ERR( "Texture '", mat.diffuse_texname, "' needs to be already in resource manager" );
                    return false;
                }
            }
        }

        Mesh currentMesh;
        glm::vec3 min = glm::vec3( std::numeric_limits< float >::max() );
        glm::vec3 max = glm::vec3( -std::numeric_limits< float >::max() );
        for ( const auto& shape : shapes )
        {
            // Loop over faces(polygon)
            for ( size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++ )
            {
                if ( shape.mesh.material_ids[f] == currentMaterialID )
                {
                    // Loop over vertices in the face. Each face should have 3 vertices from the
                    // LoadObj triangulation
                    for ( size_t v = 0; v < 3; v++ )
                    {
                        tinyobj::index_t idx = shape.mesh.indices[3 * f + v];
                        tinyobj::real_t vx   = attrib.vertices[3 * idx.vertex_index + 0];
                        tinyobj::real_t vy   = attrib.vertices[3 * idx.vertex_index + 1];
                        tinyobj::real_t vz   = attrib.vertices[3 * idx.vertex_index + 2];
                        glm::vec3 vert( vx, vy, vz );
                        min = glm::min( min, vert );
                        max = glm::max( max, vert );

                        tinyobj::real_t nx = 0;
                        tinyobj::real_t ny = 0;
                        tinyobj::real_t nz = 0;
                        if ( idx.normal_index != -1 )
                        {
                            nx = attrib.normals[3 * idx.normal_index + 0];
                            ny = attrib.normals[3 * idx.normal_index + 1];
                            nz = attrib.normals[3 * idx.normal_index + 2];
                        }

                        tinyobj::real_t tx = 0, ty = 0;
                        if ( idx.texcoord_index != -1 )
                        {
                            tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                            ty = attrib.texcoords[2 * idx.texcoord_index + 1];
                        }

                        currentMesh.vertices.emplace_back( vert );
                        
                        if ( idx.normal_index != -1 )
                        {
                            currentMesh.normals.emplace_back( nx, ny, nz );
                        }

                        if ( idx.texcoord_index != -1 )
                        {
                            currentMesh.uvs.emplace_back( tx, ty );
                        }

                        currentMesh.indices.push_back( static_cast< uint32_t >( currentMesh.indices.size() ) );
                    }
                }
            }
        }

        // create mesh and upload to GPU
        if ( currentMesh.vertices.size() )
        {
            if ( currentMaterial->name == "default" )
            {
                LOG_WARN( "Mesh from OBJ '", createInfo->filename, "' has no assigned material. Using default." );
            }

            if ( currentMesh.normals.empty() )
            {
                const auto& verts   = currentMesh.vertices;
                const auto& indices = currentMesh.indices;
                auto& normals       = currentMesh.normals;
                normals.resize( verts.size(), glm::vec3( 0 ) );

                for ( size_t i = 0; i < indices.size(); i += 3 )
                {
                    glm::vec3 v1 = verts[indices[i + 0]];
                    glm::vec3 v2 = verts[indices[i + 1]];
                    glm::vec3 v3 = verts[indices[i + 2]];
                    glm::vec3 n = glm::cross( v2 - v1, v3 - v1 );
                    normals[indices[i + 0]] += n;
                    normals[indices[i + 1]] += n;
                    normals[indices[i + 2]] += n;
                }

                for ( auto& normal : normals )
                {
                    normal = glm::normalize( normal );
                }
            }

            currentMesh.aabb = AABB( min, max );
            aabb.min         = glm::min( min, aabb.min );
            aabb.max         = glm::max( max, aabb.max );

            if ( createInfo->optimize )
            {
                currentMesh.Optimize();
            }

            if ( createInfo->createGpuCopy )
            {
                currentMesh.UploadToGpu( createInfo->freeCpuCopy );
            }

            materials.push_back( currentMaterial );
            meshes.push_back( std::move( currentMesh ) );
        }
    }

    aabb = AABB( aabb.min, aabb.max );

    return true;
}

void Model::RecalculateBB( bool recursive )
{
    if ( meshes.empty() )
    {
        return;
    }
    
    
    if ( recursive )
    {
        for ( auto& mesh : meshes )
        {
            mesh.RecalculateBB();
        }
    }

    glm::vec3 min = meshes[0].aabb.min;
    glm::vec3 max = meshes[0].aabb.max;

    for ( size_t i = 1; meshes.size(); ++i )
    {
        min = glm::min( min, meshes[i].aabb.min );
        max = glm::min( max, meshes[i].aabb.min );
    }

    aabb = AABB( min, max );
}

void Model::Optimize()
{
    for ( Mesh& mesh : meshes )
    {
        mesh.Optimize();
    }
}

} // namespace Progression
