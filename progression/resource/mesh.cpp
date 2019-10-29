#include "resource/mesh.hpp"

#include "core/assert.hpp"
#include "graphics/graphics_api.hpp"
#include "meshoptimizer/src/meshoptimizer.h"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#include "graphics/vulkan.hpp"

class Vertex
{
public:
    Vertex() : vertex( glm::vec3( 0 ) ), normal( glm::vec3( 0 ) ), uv( glm::vec3( 0 ) )
    {
    }

    Vertex( const glm::vec3& vert, const glm::vec3& norm, const glm::vec2& tex ) :
      vertex( vert ), normal( norm ), uv( tex )
    {
    }

    bool operator==( const Vertex& other ) const
    {
        return vertex == other.vertex && normal == other.normal && uv == other.uv;
    }

    glm::vec3 vertex;
    glm::vec3 normal;
    glm::vec2 uv;
};

namespace Progression
{

void Mesh::Optimize()
{
    if ( vertices.size() == 0 )
    {
        LOG_ERR(
          "Trying to optimize a mesh with no vertices. Did you free them after uploading to the "
          "GPU?" );
        return;
    }
    // collect everything back into interleaved data
    std::vector< Vertex > original_vertices;
    original_vertices.reserve( vertices.size() );
    for ( size_t i = 0; i < vertices.size(); ++i )
    {
        Vertex v;
        v.vertex = vertices[i];
        v.normal = normals[i];
        if ( uvs.size() )
        {
            v.uv = uvs[i];
        }

        original_vertices.push_back( v );
    }

    // const size_t kCacheSize = 16;
    // meshopt_VertexCacheStatistics vcs = meshopt_analyzeVertexCache( &mesh.indices[0],
    // mesh.indices.size(), mesh.vertices.size(), kCacheSize, 0, 0);
    // meshopt_OverdrawStatistics os = meshopt_analyzeOverdraw(&mesh.indices[0],
    // mesh.indices.size(), &vertices[0].vertex.x, mesh.vertices.size(), sizeof(Vertex));
    // meshopt_VertexFetchStatistics vfs = meshopt_analyzeVertexFetch(&mesh.indices[0],
    // mesh.indices.size(), mesh.vertices.size(), sizeof(Vertex) );
    // LOG( "Before:" );
    // LOG( "ACMR: ", vcs.acmr, ", ATVR: ", vcs.atvr, ", avg overdraw: ", os.overdraw, " avg # fetched: ", vfs.overfetch);
    
    size_t index_count = indices.size();
    std::vector< uint32_t > remap( index_count );
    size_t vertex_count = meshopt_generateVertexRemap( &remap[0], NULL, index_count,
            &original_vertices[0], index_count, sizeof( Vertex ) );
    meshopt_remapIndexBuffer( &indices[0], NULL, index_count, &remap[0] );
    std::vector< Vertex > opt_vertices( vertex_count );
    meshopt_remapVertexBuffer( &opt_vertices[0], &original_vertices[0], index_count, sizeof( Vertex ), &remap[0] );

    // vertex cache optimization should go first as it provides starting order for overdraw
    meshopt_optimizeVertexCache( &indices[0], &indices[0], indices.size(), opt_vertices.size() );

    // reorder indices for overdraw, balancing overdraw and vertex cache efficiency
    const float kThreshold = 1.01f; // allow up to 1% worse ACMR to get more reordering opportunities for overdraw
    meshopt_optimizeOverdraw( &indices[0], &indices[0], indices.size(), &opt_vertices[0].vertex.x,
                              opt_vertices.size(), sizeof( Vertex ), kThreshold );

    // vertex fetch optimization should go last as it depends on the final index order
    meshopt_optimizeVertexFetch( &opt_vertices[0].vertex.x, &indices[0], indices.size(),
                                 &opt_vertices[0].vertex.x, opt_vertices.size(), sizeof( Vertex ) );

    // vcs = meshopt_analyzeVertexCache(&indices[0], indices.size(), vertices.size(), kCacheSize, 0,
    // 0); os = meshopt_analyzeOverdraw(&indices[0], indices.size(), &vertices[0].vertex.x,
    // vertices.size(), sizeof(Vertex)); vfs = meshopt_analyzeVertexFetch(&indices[0],
    // indices.size(), vertices.size(), sizeof(Vertex)); LOG("After:"); LOG("ACMR: ", vcs.acmr, ",
    // ATVR: ", vcs.atvr, ", avg overdraw: ", os.overdraw, " avg # fetched: ", vfs.overfetch);

    // collect back into mesh structure
    for ( size_t i = 0; i < opt_vertices.size(); ++i )
    {
        const auto& v = opt_vertices[i];
        vertices[i]   = v.vertex;
        if ( normals.size() ) 
        {
            normals[i] = v.normal;
        }

        if ( uvs.size() )
        {
            uvs[i] = v.uv;
        }
    }
}

// TODO: dynamic meshes + usage
void Mesh::UploadToGpu( bool freeCPUCopy )
{
    using namespace Gfx;
    PG_ASSERT( indices.size() != 0 && vertices.size() == normals.size() );
    if ( m_gpuDataCreated )
    {
        vertexBuffer.Free();
        indexBuffer.Free();
    }
    m_gpuDataCreated = true;
    std::vector< float > vertexData( 3 * vertices.size() + 3 * normals.size() + 2 * uvs.size() );
    char* dst = (char*) vertexData.data();
    memcpy( dst, vertices.data(), vertices.size() * sizeof( glm::vec3 ) );
    dst += vertices.size() * sizeof( glm::vec3 );
    memcpy( dst, normals.data(), normals.size() * sizeof( glm::vec3 ) );
    dst += normals.size() * sizeof( glm::vec3 );
    memcpy( dst, uvs.data(), uvs.size() * sizeof( glm::vec2 ) );

    vertexBuffer = Gfx::g_renderState.device.NewBuffer( vertexData.size() * sizeof( float ), vertexData.data(), BUFFER_TYPE_VERTEX, MEMORY_TYPE_DEVICE_LOCAL );
    indexBuffer  = Gfx::g_renderState.device.NewBuffer( indices.size() * sizeof ( uint32_t ), indices.data(), BUFFER_TYPE_INDEX, MEMORY_TYPE_DEVICE_LOCAL );

    m_numVertices  = static_cast< uint32_t >( vertices.size() );
    m_numIndices   = static_cast< uint32_t >( indices.size() );
    m_normalOffset = m_numVertices * sizeof( glm::vec3 );
    m_uvOffset     = m_normalOffset + m_numVertices * sizeof( glm::vec3 );

    Free( false, freeCPUCopy );
}

void Mesh::RecalculateBB()
{
    PG_ASSERT( vertices.size() != 0 );
    glm::vec3 max = vertices[0];
    glm::vec3 min = vertices[0];
    for ( size_t i = 1; i < vertices.size(); ++i )
    {
        min = glm::min( min, vertices[i] );
        max = glm::max( max, vertices[i] );
    }

    aabb = AABB( min, max );
}

void Mesh::RecalculateNormals()
{
    normals.resize( vertices.size(), glm::vec3( 0 ) );

    for ( size_t i = 0; i < indices.size(); i += 3 )
    {
        glm::vec3 v1 = vertices[indices[i + 0]];
        glm::vec3 v2 = vertices[indices[i + 1]];
        glm::vec3 v3 = vertices[indices[i + 2]];
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

void Mesh::Free( bool gpuCopy, bool cpuCopy )
{
    if ( cpuCopy )
    {
        m_numVertices = static_cast< uint32_t >( vertices.size() );
        m_numIndices  = static_cast< uint32_t >( indices.size() );
        vertices.shrink_to_fit();
        normals.shrink_to_fit();
        uvs.shrink_to_fit();
        indices.shrink_to_fit();
    }

    if ( gpuCopy )
    {
        vertexBuffer.Free();
        indexBuffer.Free();
        m_numVertices = m_numIndices = 0;
        m_normalOffset = m_uvOffset = ~0u;
    }
}

bool Mesh::Serialize( std::ofstream& out ) const
{
    if ( !vertices.size() || !normals.size() )
    {
        LOG_ERR( "Need CPU vertices and normals to save! Was this mesh accidentally uploaded to the GPU + freed?" );
        return false;
    }

    size_t totalVertexBytes = sizeof( glm::vec3 ) * vertices.size() + sizeof( glm::vec3 ) * normals.size() + sizeof( glm::vec2 ) * uvs.size();
    size_t totalIndexBytes  = sizeof( uint32_t ) * indices.size();
    serialize::Write( out, totalVertexBytes );
    serialize::Write( out, totalIndexBytes );
    serialize::Write( out, vertices.size() );
    serialize::Write( out, normals.size() );
    serialize::Write( out, uvs.size() );
    serialize::Write( out, (char*) vertices.data(), vertices.size() * sizeof( glm::vec3 ) );
    serialize::Write( out, (char*) normals.data(),  normals.size()  * sizeof( glm::vec3 ) );
    serialize::Write( out, (char*) uvs.data(),      uvs.size()      * sizeof( glm::vec2 ) );
    serialize::Write( out, Gfx::IndexType::UNSIGNED_INT );
    serialize::Write( out, indices );
    serialize::Write( out, aabb.min );
    serialize::Write( out, aabb.max );
    serialize::Write( out, aabb.extent );
    
    return !out.fail();
}

bool Mesh::Deserialize( char*& buffer, bool createGpuCopy, bool freeCpuCopy )
{
    using namespace Gfx;
    PG_ASSERT( createGpuCopy || !freeCpuCopy );
    size_t totalVertexBytes, totalIndexBytes, buffSize;
    serialize::Read( buffer, totalVertexBytes );
    serialize::Read( buffer, totalIndexBytes );

    if ( createGpuCopy )
    {
        m_gpuDataCreated = true;
        size_t numVertices, numNormals, numUVs;
        serialize::Read( buffer, numVertices );
        serialize::Read( buffer, numNormals );
        serialize::Read( buffer, numUVs );
        m_numVertices  = static_cast< uint32_t >( numVertices );
        m_normalOffset = m_numVertices * sizeof( glm::vec3 ); 
        m_uvOffset     = static_cast< uint32_t >( m_normalOffset + numNormals * sizeof( glm::vec3 ) ); 
        
        vertexBuffer = Gfx::g_renderState.device.NewBuffer( totalVertexBytes, buffer, BUFFER_TYPE_VERTEX, MEMORY_TYPE_DEVICE_LOCAL );
        if ( !freeCpuCopy )
        {
            vertices.resize( numVertices );
            normals.resize( numNormals );
            uvs.resize( numUVs );
            serialize::Read( buffer, (char*) vertices.data(), normals.size() * sizeof( glm::vec3 ) );
            serialize::Read( buffer, (char*) normals.data(), vertices.size() * sizeof( glm::vec3 ) );
            serialize::Read( buffer, (char*) uvs.data(), uvs.size() * sizeof( glm::vec2 ) );
        }
        else
        {
            buffer += totalVertexBytes;
        }

        serialize::Read( buffer, m_indexType );
        serialize::Read( buffer, buffSize );
        m_numIndices = static_cast< uint32_t >( buffSize );
        indexBuffer  = Gfx::g_renderState.device.NewBuffer( totalIndexBytes, buffer, BUFFER_TYPE_INDEX, MEMORY_TYPE_DEVICE_LOCAL );
        if ( !freeCpuCopy )
        {
            indices.resize( m_numIndices );
            serialize::Read( buffer, (char*) indices.data(), totalIndexBytes );
        }
        else
        {
            buffer += totalIndexBytes;
        }
    }
    else
    {
        m_gpuDataCreated = false;

        serialize::Read( buffer, buffSize );
        vertices.resize( buffSize );
        serialize::Read( buffer, buffSize );
        normals.resize( buffSize );
        serialize::Read( buffer, buffSize );
        uvs.resize( buffSize );
        serialize::Read( buffer, (char*) vertices.data(), normals.size() * sizeof( glm::vec3 ) );
        serialize::Read( buffer, (char*) normals.data(), vertices.size() * sizeof( glm::vec3 ) );
        serialize::Read( buffer, (char*) uvs.data(), uvs.size() * sizeof( glm::vec2 ) );
        serialize::Read( buffer, m_indexType );
        serialize::Read( buffer, indices );
        m_numVertices = static_cast< uint32_t >( vertices.size() );
        m_numIndices  = static_cast< uint32_t >( indices.size() );
    }

    serialize::Read( buffer, aabb.min );
    serialize::Read( buffer, aabb.max );
    serialize::Read( buffer, aabb.extent );

    return true;
}

uint32_t Mesh::GetNumVertices() const
{
    return m_numVertices;
}

uint32_t Mesh::GetNumIndices() const
{
    return m_numIndices;
}

uint32_t Mesh::GetVertexOffset() const
{
    return 0;
}

uint32_t Mesh::GetNormalOffset() const
{
    return m_normalOffset;
}

uint32_t Mesh::GetUVOffset() const
{
    return m_uvOffset;
}

Gfx::IndexType Mesh::GetIndexType() const
{
    return m_indexType;
}

} // namespace Progression
