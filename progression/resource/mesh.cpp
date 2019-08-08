#include "resource/mesh.hpp"
#include "graphics/graphics_api.hpp"
#include "meshoptimizer/src/meshoptimizer.h"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

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

namespace std
{
template <>
struct hash< Vertex >
{
    size_t operator()( Vertex const& vertex ) const
    {
        return ( ( hash< glm::vec3 >()( vertex.vertex ) ^
                   ( hash< glm::vec3 >()( vertex.normal ) << 1 ) ) >>
                 1 ) ^
               ( hash< glm::vec2 >()( vertex.uv ) << 1 );
    }
};
} // namespace std


namespace Progression
{

Mesh::Mesh( Mesh&& mesh )
{
    *this = std::move( mesh );
}

Mesh& Mesh::operator=( Mesh&& mesh )
{
    vertices         = std::move( mesh.vertices );
    normals          = std::move( mesh.normals );
    uvs              = std::move( mesh.uvs );
    indices          = std::move( mesh.indices );
    m_numVertices    = std::move( mesh.m_numVertices );
    m_numIndices     = std::move( mesh.m_numIndices );
    m_normalOffset   = std::move( mesh.m_normalOffset );
    m_uvOffset       = std::move( mesh.m_uvOffset );
    m_gpuDataCreated = std::move( mesh.m_gpuDataCreated );
    vertexBuffer     = std::move( mesh.vertexBuffer );
    indexBuffer      = std::move( mesh.indexBuffer );

    return *this;
}

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
    std::vector< Vertex > opt_vertices;
    for ( size_t i = 0; i < vertices.size(); ++i )
    {
        Vertex v;
        v.vertex = vertices[i];
        if ( normals.size() )
            v.normal = normals[i];
        if ( uvs.size() )
            v.uv = uvs[i];

        opt_vertices.push_back( v );
    }

    // const size_t kCacheSize = 16;
    // meshopt_VertexCacheStatistics vcs = meshopt_analyzeVertexCache(&mesh.indices[0],
    // mesh.indices.size(),
    //         mesh.vertices.size(), kCacheSize, 0, 0);
    // meshopt_OverdrawStatistics os = meshopt_analyzeOverdraw(&mesh.indices[0],
    // mesh.indices.size(),
    //         &vertices[0].vertex.x, mesh.vertices.size(), sizeof(Vertex));
    // meshopt_VertexFetchStatistics vfs = meshopt_analyzeVertexFetch(&mesh.indices[0],
    // mesh.indices.size(),
    //         mesh.vertices.size(), sizeof(Vertex));
    // LOG("Before:");
    // LOG("ACMR: ", vcs.acmr, ", ATVR: ", vcs.atvr, ", avg overdraw: ", os.overdraw, " avg #
    // fetched: ", vfs.overfetch);

    // vertex cache optimization should go first as it provides starting order for overdraw
    meshopt_optimizeVertexCache( &indices[0], &indices[0], indices.size(), opt_vertices.size() );

    // reorder indices for overdraw, balancing overdraw and vertex cache efficiency
    const float kThreshold =
      1.01f; // allow up to 1% worse ACMR to get more reordering opportunities for overdraw
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
            normals[i] = v.normal;
        if ( uvs.size() )
            uvs[i] = v.uv;
    }
}

// TODO: dynamic meshes + usage
void Mesh::UploadToGpu( bool freeCPUCopy )
{
    if ( !m_gpuDataCreated )
    {
        m_gpuDataCreated = true;
        size_t vertsSize = sizeof( glm::vec3 ) * vertices.size() +
                           sizeof( glm::vec3 ) * normals.size() + sizeof( glm::vec2 ) * uvs.size();
        vertexBuffer =
          Gfx::Buffer::Create( NULL, vertsSize, Gfx::BufferType::VERTEX, Gfx::BufferUsage::STATIC );

        indexBuffer = Gfx::Buffer::Create( NULL, indices.size() * sizeof( unsigned int ),
                                           Gfx::BufferType::INDEX, Gfx::BufferUsage::STATIC );
    }

    m_numVertices  = vertices.size();
    m_numIndices   = indices.size();
    m_normalOffset = m_numVertices * sizeof( glm::vec3 );
    m_uvOffset     = m_normalOffset + normals.size() * sizeof( glm::vec3 );

    vertexBuffer.SetData( vertices.data(), 0, vertices.size() * sizeof( glm::vec3 ) );
    vertexBuffer.SetData( normals.data(), m_normalOffset, normals.size() * sizeof( glm::vec3 ) );
    vertexBuffer.SetData( uvs.data(), m_uvOffset, uvs.size() * sizeof( glm::vec2 ) );
    indexBuffer.SetData( indices.data(), indices.size() * sizeof( uint32_t ) );

    Free( false, freeCPUCopy );
}

void Mesh::Free( bool gpuCopy, bool cpuCopy )
{
    if ( cpuCopy )
    {
        m_numVertices = vertices.size();
        m_numIndices  = indices.size();
        vertices.shrink_to_fit();
        normals.shrink_to_fit();
        uvs.shrink_to_fit();
        indices.shrink_to_fit();
    }

    if ( gpuCopy )
    {
        vertexBuffer  = Gfx::Buffer{};
        indexBuffer   = Gfx::Buffer{};
        m_numVertices = m_numIndices = 0;
        m_normalOffset = m_uvOffset = ~0u;
    }
}

bool Mesh::Serialize( std::ofstream& out ) const
{
    if ( !vertices.size() )
    {
        LOG_ERR(
          "No vertex data to save. Was this mesh accidentally uploaded to the GPU + freed?" );
        return false;
    }

    serialize::Write( out, vertices );
    serialize::Write( out, normals );
    serialize::Write( out, uvs );
    serialize::Write( out, indices );

    return !out.fail();
}

bool Mesh::Deserialize( std::ifstream& in )
{
    m_gpuDataCreated = false;
    serialize::Read( in, vertices );
    serialize::Read( in, normals );
    serialize::Read( in, uvs );
    serialize::Read( in, indices );

    return !in.fail();
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

} // namespace Progression
