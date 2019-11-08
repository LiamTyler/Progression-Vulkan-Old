#pragma once

#include "core/bounding_box.hpp"
#include "core/math.hpp"
#include "graphics/graphics_api/buffer.hpp"
#include "resource/material.hpp"
#include <vector>
#include <unordered_map>

namespace Progression
{

    struct BlendWeight
    {
        glm::vec4 weights = glm::vec4( 0 );
        glm::uvec4 joints = glm::uvec4( 0 );

        void AddJointData( uint32_t id, float w )
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
            int min = 0;
			for ( int m = 1; m < 4; ++m )
			{
				if ( weights[m] < weights[min] )
                {
                    min = m;
                }
			}

			if ( weights[min] < w )
			{
				weights[min] = w;
				joints[min] = id;
			}
        }
    };

    struct Joint
    {
        std::string name;
        glm::mat4 inverseBindTransform;
        std::vector< uint32_t > children;
    };

    struct JointTransform
    {
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 scale;

        glm::mat4 GetLocalTransformMatrix() const;
        JointTransform Interpolate( const JointTransform& end, float progress );
    };

    struct KeyFrame
    {
        std::vector< JointTransform > jointSpaceTransforms; // one element for each bone in the model's skeleton. Same ordering as skeleton
        float time;
    };

    struct Animation
    {
        std::string name;
        float duration;
        float ticksPerSecond;
        std::vector< KeyFrame > keyFrames;
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
        std::vector< BlendWeight > blendWeights;
        Gfx::Buffer vertexBuffer;
        Gfx::Buffer indexBuffer;

        AABB aabb;
        std::vector< SkinnedMesh > meshes;
        std::vector< std::shared_ptr< Material > > materials;

        std::vector< Joint > joints;

        static bool LoadFBX( const std::string& filename, std::shared_ptr< SkinnedModel >& model, std::vector< Animation >& animations );
        void RecalculateNormals();
        void RecalculateAABB();
        void UploadToGpu();
        void Free( bool cpuCopy = true, bool gpuCopy = false );

        void ApplyPoseToJoints( uint32_t jointIdx, const glm::mat4& parentTransform, std::vector< glm::mat4 >& transformBuffer );

        uint32_t GetNumVertices() const;
        uint32_t GetVertexOffset() const;
        uint32_t GetNormalOffset() const;
        uint32_t GetUVOffset() const;
        uint32_t GetBlendWeightOffset() const;
        Gfx::IndexType GetIndexType() const;

    private:
        uint32_t m_numVertices          = 0;
        uint32_t m_normalOffset         = ~0u;
        uint32_t m_uvOffset             = ~0u;
        uint32_t m_blendWeightOffset    = ~0u;
        bool m_gpuDataCreated           = false;
    };

} // namespace Progression