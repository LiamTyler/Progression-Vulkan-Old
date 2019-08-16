#pragma once

#include "core/common.hpp"
#include "graphics/graphics_api.hpp"
#include "utils/noncopyable.hpp"
#include "core/bounding_box.hpp"
#include <vector>

namespace Progression
{

class Mesh : public NonCopyable
{
public:
    Mesh()  = default;
    ~Mesh() = default;

    Mesh( Mesh&& mesh ) = default;
    Mesh& operator=( Mesh&& mesh ) = default;

    void UploadToGpu( bool freeCPUCopy = true );
    void RecalculateBB();
    void Free( bool gpuCopy = true, bool cpuCopy = true );
    void Optimize();
    bool Serialize( std::ofstream& out ) const;
    bool Deserialize( char*& buffer );

    uint32_t GetNumVertices() const;
    uint32_t GetNumIndices() const;
    uint32_t GetVertexOffset() const;
    uint32_t GetNormalOffset() const;
    uint32_t GetUVOffset() const;
    Gfx::IndexType GetIndexType() const;
    // void SetKeepCPUCopyOnDeserialize( bool b );

    AABB aabb;
    std::vector< glm::vec3 > vertices;
    std::vector< glm::vec3 > normals;
    std::vector< glm::vec2 > uvs;
    std::vector< uint32_t > indices;
    Gfx::Buffer vertexBuffer;
    Gfx::Buffer indexBuffer;

private:
    Gfx::IndexType m_indexType = Gfx::IndexType::UNSIGNED_INT;
    uint32_t m_numVertices     = 0;
    uint32_t m_numIndices      = 0;
    uint32_t m_normalOffset    = ~0u;
    uint32_t m_uvOffset        = ~0u;
    bool m_gpuDataCreated      = false;
    bool m_keepCPUCopy         = false;
};

} // namespace Progression
