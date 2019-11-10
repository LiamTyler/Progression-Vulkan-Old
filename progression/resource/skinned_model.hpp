#pragma once

#include "core/bounding_box.hpp"
#include "core/math.hpp"
#include "graphics/graphics_api/buffer.hpp"
#include "resource/material.hpp"
#include "resource/resource.hpp"
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
        std::vector< JointTransform > jointSpaceTransforms; // one element for each joint in the model's skeleton. Same ordering as skeleton
        float time;
    };

    struct Animation
    {
        std::string name;
        float duration;
        float ticksPerSecond;
        std::vector< KeyFrame > keyFrames;
    };

    struct SkinnedMesh
    {
    public:
        int materialIndex = -1;
        uint32_t startIndex  = ~0u;
        uint32_t startVertex = ~0u;
        uint32_t numIndices  = ~0u;
    };

    struct Skeleton
    {
        std::vector< Joint > joints;

        void Serialize( std::ofstream& outFile ) const;
        void Deserialize( char*& buffer );
    };

    struct SkinnedModelCreateInfo : public ResourceCreateInfo
    {
        std::string filename = "";
        bool optimize        = true;
        bool freeCpuCopy     = true;
        bool createGpuCopy   = true;
    };

    class SkinnedModel : public Resource
    {
    public:
        SkinnedModel() = default;
        SkinnedModel( SkinnedModel&& model ) = default;
        SkinnedModel& operator=( SkinnedModel&& model ) = default;
        
        bool Load( ResourceCreateInfo* createInfo = nullptr ) override;
        void Move( std::shared_ptr< Resource > dst ) override;
        bool Serialize( std::ofstream& outFile ) const override;
        bool Deserialize( char*& buffer ) override;

        void RecalculateNormals();
        void RecalculateAABB();
        void UploadToGpu();
        void FreeGeometry( bool cpuCopy = true, bool gpuCopy = false );
        void Optimize();

        void ApplyPoseToJoints( uint32_t jointIdx, const glm::mat4& parentTransform, std::vector< glm::mat4 >& transformBuffer );

        uint32_t GetNumVertices() const;
        uint32_t GetVertexOffset() const;
        uint32_t GetNormalOffset() const;
        uint32_t GetUVOffset() const;
        uint32_t GetBlendWeightOffset() const;
        Gfx::IndexType GetIndexType() const;

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
        Skeleton skeleton;
        std::vector< Animation > animations;

    private:
        uint32_t m_numVertices       = 0;
        uint32_t m_normalOffset      = ~0u;
        uint32_t m_uvOffset          = ~0u;
        uint32_t m_blendWeightOffset = ~0u;
        bool m_gpuDataCreated        = false;
    };

} // namespace Progression