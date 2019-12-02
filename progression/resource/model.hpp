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

        void AddJointData( uint32_t id, float w );
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

    struct Mesh
    {
    public:
        std::string name;
        int materialIndex = -1;
        uint32_t startIndex  = 0;
        uint32_t numIndices  = 0;
        uint32_t startVertex = 0;
        uint32_t numVertices = 0;
    };

    struct Skeleton
    {
        std::vector< Joint > joints;

        void Serialize( std::ofstream& outFile ) const;
        void Deserialize( char*& buffer );
    };

    struct ModelCreateInfo : public ResourceCreateInfo
    {
        std::string filename = "";
        bool optimize        = true;
        bool freeCpuCopy     = true;
        bool createGpuCopy   = true;
    };

    class Model : public Resource
    {
    public:
        Model() = default;
        ~Model();
        Model( Model&& model ) = default;
        Model& operator=( Model&& model ) = default;
        
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
        uint32_t GetTangentOffset() const;
        uint32_t GetBlendWeightOffset() const;
        Gfx::IndexType GetIndexType() const;

        std::vector< glm::vec3 > vertices;
        std::vector< glm::vec3 > normals;
        std::vector< glm::vec2 > uvs;
        std::vector< BlendWeight > blendWeights;
        std::vector< glm::vec3 > tangents;
        std::vector< uint32_t > indices;
        Gfx::Buffer vertexBuffer;
        Gfx::Buffer indexBuffer;

        AABB aabb;
        std::vector< Mesh > meshes;
        std::vector< std::shared_ptr< Material > > materials;
        Skeleton skeleton;
        std::vector< Animation > animations;

    private:
        uint32_t m_numVertices       = 0;
        uint32_t m_normalOffset      = ~0u;
        uint32_t m_uvOffset          = ~0u;
        uint32_t m_blendWeightOffset = ~0u;
        uint32_t m_tangentOffset     = ~0u;
    };

} // namespace Progression