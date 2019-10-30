#pragma once

#include "core/bounding_box.hpp"
#include "core/math.hpp"
#include "resource/material.hpp"
#include "graphics/graphics_api/buffer.hpp"
#include "assimp/Importer.hpp"      // C++ importer interface
#include "assimp/postprocess.h" // Post processing flags
#include "assimp/scene.h"       // Output data structure
#include <vector>
#include <unordered_map>

namespace Progression
{

    struct VertexBoneData
    {
        glm::vec4 weights = glm::vec4( 0 );
        glm::uvec4 joints = glm::uvec4( 0 );

        void AddBoneData( uint32_t id, float w )
        {
            for ( uint32_t i = 0; i < 4; i++)
            {
                if ( weights[i] == 0.0 )
                {
                    joints[i]  = id;
                    weights[i] = w;
                    return;
                }        
            }
        }
    };

    struct BoneData
    {
        std::string name;
        glm::mat4 offset              = glm::mat4( 1 );
        glm::mat4 finalTransformation = glm::mat4( 1 );
        uint32_t parentIndex          = ~0u;
        std::vector< uint32_t > children;
    };

    class SkinnedMesh
    {
        friend class SkinnedModel;
    public:
        uint32_t GetStartVertex() const;
        uint32_t GetStartIndex() const;
        uint32_t GetNumIndices() const;

        int materialIndex = -1;

    private:
        uint32_t m_startIndex  = ~0u;
        uint32_t m_startVertex = ~0u;
        uint32_t m_numIndices  = ~0u;
    };

    class SkinnedModel
    {
    public:
        std::string name;
        std::vector< glm::vec3 > vertices;
        std::vector< glm::vec3 > normals;
        std::vector< glm::vec2 > uvs;
        std::vector< uint32_t > indices;
        std::vector< VertexBoneData > vertexBoneData;
        std::vector< BoneData > bones;

        Gfx::Buffer vertexBuffer;
        Gfx::Buffer indexBuffer;

        std::vector< SkinnedMesh > meshes;

        static bool LoadFBX( const std::string& filename, std::shared_ptr< SkinnedModel >& model );
        void RecalculateNormals();
        void RecalculateAABB();
        void UploadToGpu();
        void Free( bool cpuCopy = true, bool gpuCopy = false );

        void TransformBones( std::vector< glm::mat4 >& finalTransforms, glm::mat4 modelMatrix = glm::mat4( 1 ) );
        void ReadNodeHeirarchy( float AnimationTime, const aiNode* pNode, const glm::mat4& ParentTransform );

        uint32_t GetNumVertices() const;
        uint32_t GetVertexOffset() const;
        uint32_t GetNormalOffset() const;
        uint32_t GetUVOffset() const;
        uint32_t GetVertexBoneDataOffset() const;
        Gfx::IndexType GetIndexType() const;

        BoneData rootBone;
        glm::mat4 globalInverseTransform;

        std::vector< std::shared_ptr< Material > > materials;

        const aiScene* m_scene;
        Assimp::Importer m_importer;
        AABB aabb;

    private:
        std::unordered_map< std::string, uint32_t > m_boneMapping;
        uint32_t m_numVertices          = 0;
        uint32_t m_normalOffset         = ~0u;
        uint32_t m_uvOffset             = ~0u;
        uint32_t m_vertexBoneDataOffset = ~0u;
        bool m_gpuDataCreated           = false;
    };

} // namespace Progression