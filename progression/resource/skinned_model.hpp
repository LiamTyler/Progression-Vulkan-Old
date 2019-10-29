#pragma once

#include "core/math.hpp"
#include "resource/material.hpp"
#include "graphics/graphics_api/buffer.hpp"
#include <vector>

namespace Progression
{

    struct VertexBoneData
    {
        glm::vec4 weights = glm::vec4( 0 );
        glm::uvec4 joints = glm::uvec4( 0 );
    };

    struct BoneData
    {
        glm::mat4 offset              = glm::mat4( 1 );
        glm::mat4 finalTransformation = glm::mat4( 1 );
        uint32_t parentIndex          = ~0u;
        std::vector< uint32_t > children;
    };

    class SkinnedMesh
    {
        friend class SkinnedModel;
    public:
        std::vector< uint32_t > indices;
        std::shared_ptr< Material > material;

        uint32_t GetStartIndex() const;
        uint32_t GetNumIndices() const;

    private:
        uint32_t m_startIndex;
        uint32_t m_numIndices;
    };

    class SkinnedModel
    {
    public:
        std::string name;
        std::vector< glm::vec3 > vertices;
        std::vector< glm::vec3 > normals;
        std::vector< glm::vec2 > uvs;
        std::vector< VertexBoneData > vertexBoneData;
        std::vector< BoneData > skeleton;

        Gfx::Buffer vertexBuffer;
        Gfx::Buffer indexBuffer;

        std::vector< SkinnedMesh > meshes;

        static bool LoadFBX( const std::string& filename, std::vector< std::shared_ptr< SkinnedModel > >& models );
        void RecalculateNormals();
        void UploadToGpu();
        void Free( bool cpuCopy = true, bool gpuCopy = false );

        void TransformBones( std::vector< glm::mat4 >& finalTransforms, glm::mat4 modelMatrix = glm::mat4( 1 ) );

        uint32_t GetNumVertices() const;
        uint32_t GetVertexOffset() const;
        uint32_t GetNormalOffset() const;
        uint32_t GetUVOffset() const;
        uint32_t GetVertexBoneDataOffset() const;
        Gfx::IndexType GetIndexType() const;

        BoneData rootBone;

    private:
        void TransformChildren( uint32_t boneIdx, const glm::mat4& parentMatrix, std::vector< glm::mat4 >& finalTransforms );
        
        uint32_t m_numVertices          = 0;
        uint32_t m_normalOffset         = ~0u;
        uint32_t m_uvOffset             = ~0u;
        uint32_t m_vertexBoneDataOffset = ~0u;
        bool m_gpuDataCreated           = false;
    };

} // namespace Progression