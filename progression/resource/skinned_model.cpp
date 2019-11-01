#include "resource/skinned_model.hpp"
#include "openFBX/ofbx.h"
#include "core/assert.hpp"
#include "core/time.hpp"
#include "utils/logger.hpp"
#include "graphics/vulkan.hpp"
#include "assimp/Importer.hpp"      // C++ importer interface
#include "assimp/postprocess.h" // Post processing flags
#include "assimp/scene.h"       // Output data structure
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"
#include <set>

static aiMatrix4x4 GLMMat4ToAi( const glm::mat4& mat )
{
    return aiMatrix4x4( mat[0][0], mat[0][1], mat[0][2], mat[0][3],
                        mat[1][0], mat[1][1], mat[1][2], mat[1][3],
                        mat[2][0], mat[2][1], mat[2][2], mat[2][3],
                        mat[3][0], mat[3][1], mat[3][2], mat[3][3] );
}

static glm::mat4 AiToGLMMat4( const aiMatrix4x4& in_mat )
{
    glm::mat4 tmp;
    tmp[0][0] = in_mat.a1;
    tmp[1][0] = in_mat.b1;
    tmp[2][0] = in_mat.c1;
    tmp[3][0] = in_mat.d1;

    tmp[0][1] = in_mat.a2;
    tmp[1][1] = in_mat.b2;
    tmp[2][1] = in_mat.c2;
    tmp[3][1] = in_mat.d2;

    tmp[0][2] = in_mat.a3;
    tmp[1][2] = in_mat.b3;
    tmp[2][2] = in_mat.c3;
    tmp[3][2] = in_mat.d3;

    tmp[0][3] = in_mat.a4;
    tmp[1][3] = in_mat.b4;
    tmp[2][3] = in_mat.c4;
    tmp[3][3] = in_mat.d4;
    return tmp;
}

static glm::vec3 AiToGLMVec3( const aiVector3D& v )
{
    return { v.x, v.y, v.z };
}

static glm::quat AiToGLMQuat( const aiQuaternion& q )
{
    return { q.x, q.y, q.z, q.w };
}

namespace Progression
{

    uint32_t SkinnedMesh::GetStartVertex() const
    {
        return m_startVertex;
    }

    uint32_t SkinnedMesh::GetStartIndex() const
    {
        return m_startIndex;
    }

    uint32_t SkinnedMesh::GetNumIndices() const
    {
        return m_numIndices;
    }

    void SkinnedModel::RecalculateNormals()
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

    void SkinnedModel::RecalculateAABB()
    {
        aabb.min = vertices[0];
        aabb.max = vertices[0];
        for ( const auto& vertex : vertices )
        {
            aabb.min = glm::min( aabb.min, vertex );
            aabb.max = glm::max( aabb.max, vertex );
        }
    }

    void SkinnedModel::UploadToGpu()
    {
        using namespace Gfx;

        if ( m_gpuDataCreated )
        {
            vertexBuffer.Free();
            indexBuffer.Free();
        }
        m_gpuDataCreated = true;
        std::vector< float > vertexData( 3 * vertices.size() + 3 * normals.size() + 2 * uvs.size() + 8 * blendWeights.size() );
        char* dst = (char*) vertexData.data();
        memcpy( dst, vertices.data(), vertices.size() * sizeof( glm::vec3 ) );
        dst += vertices.size() * sizeof( glm::vec3 );
        memcpy( dst, normals.data(), normals.size() * sizeof( glm::vec3 ) );
        dst += normals.size() * sizeof( glm::vec3 );
        memcpy( dst, uvs.data(), uvs.size() * sizeof( glm::vec2 ) );
        dst += uvs.size() * sizeof( glm::vec2 );
        memcpy( dst, blendWeights.data(), blendWeights.size() * 2 * sizeof( glm::vec4 ) );
        LOG( "Num floats = ", vertexData.size() );
        vertexBuffer = Gfx::g_renderState.device.NewBuffer( vertexData.size() * sizeof( float ), vertexData.data(), BUFFER_TYPE_VERTEX, MEMORY_TYPE_DEVICE_LOCAL );
        indexBuffer  = Gfx::g_renderState.device.NewBuffer( indices.size() * sizeof ( uint32_t ), indices.data(), BUFFER_TYPE_INDEX, MEMORY_TYPE_DEVICE_LOCAL );

        m_numVertices           = static_cast< uint32_t >( vertices.size() );
        m_normalOffset          = m_numVertices * sizeof( glm::vec3 );
        m_uvOffset              = m_normalOffset + m_numVertices * sizeof( glm::vec3 );
        m_blendWeightOffset     = static_cast< uint32_t >( m_uvOffset + uvs.size() * sizeof( glm::vec2 ) );

        LOG( "m_numVertices = ",    m_numVertices );
        LOG( "m_normalOffset = ",   m_normalOffset );
        LOG( "m_uvOffset = ",       m_uvOffset );
        LOG( "m_blendWeightOffset = ", m_blendWeightOffset );
        LOG( "normals.size() = ",   normals.size() );
        LOG( "uvs.size() = ",       uvs.size() );

        // Free();
    }

    void SkinnedModel::Free( bool cpuCopy, bool gpuCopy )
    {
        if ( cpuCopy )
        {
            m_numVertices = static_cast< uint32_t >( vertices.size() );
            vertices      = std::vector< glm::vec3 >();
            normals       = std::vector< glm::vec3 >();
            uvs           = std::vector< glm::vec2 >();
            indices       = std::vector< uint32_t >();
            blendWeights  = std::vector< BlendWeight >();
            joints        = std::vector< Joint >();
            animations    = std::vector< Animation >();
        }

        if ( gpuCopy )
        {
            vertexBuffer.Free();
            indexBuffer.Free();
            m_numVertices = 0;
            m_normalOffset = m_uvOffset = m_blendWeightOffset = ~0u;
        }
    }

    glm::mat4 JointTransform::GetLocalTransformMatrix() const
    {
        glm::mat4 T = glm::translate( glm::mat4( 1 ), position );
        glm::mat4 R = glm::toMat4( rotation );
        glm::mat4 S = glm::scale( glm::mat4( 1 ), scale );
        return T * R * S;
    }

    aiNodeAnim* FindAINodeAnim( aiAnimation* pAnimation, const std::string& NodeName )
    {
        for ( uint32_t i = 0; i < pAnimation->mNumChannels; ++i )
        {
            aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];
        
            if ( std::string( pNodeAnim->mNodeName.data ) == NodeName )
            {
                return pNodeAnim;
            }
        }
    
        return NULL;
    }

    static JointTransform InterpolateJoints( const JointTransform& start, const JointTransform& end, float t )
    {
        JointTransform ret;
        ret.position = ( 1.0f - t ) * start.position + t * end.position;
        ret.rotation = glm::mix( start.rotation, end.rotation, t );
        ret.scale    = ( 1.0f - t ) * start.position + t * end.scale;

        return ret;
    }

    static void ReadNodeHeirarchy( aiNode* pNode, uint32_t numAnimations, aiAnimation** animations, int depth = 0 )
    {
        std::string NodeName( pNode->mName.data );
        std::string tabbing( depth * 2, ' ' );
            
        glm::mat4 NodeTransformation( AiToGLMMat4( pNode->mTransformation ) );

        bool animFound = false;
        for ( auto i = 0u; i < numAnimations && !animFound; ++i )
        {
            aiNodeAnim* pNodeAnim = FindAINodeAnim( animations[i], NodeName );
            if ( pNodeAnim )
            {
                LOG( tabbing, "Node '", NodeName, "' has first animation component in animation: ", i );
                animFound = true;
            }
        }
        if ( !animFound )
        {
            LOG( tabbing, "Node '", NodeName, "' does not have animation component" );
        }
        LOG( tabbing, "NodeTransformation =" );
        LOG( tabbing, NodeTransformation[0] );
        LOG( tabbing, NodeTransformation[1] );
        LOG( tabbing, NodeTransformation[2] );
        LOG( tabbing, NodeTransformation[3] );
        
    
        for ( uint32_t i = 0; i < pNode->mNumChildren; i++ )
        {
            ReadNodeHeirarchy( pNode->mChildren[i], numAnimations, animations, depth + 1 );
        }
    }

    static void FindJointChildren( aiNode* node, std::unordered_map< std::string, uint32_t >& jointMapping, std::vector< Joint >& joints )
    {
        std::string name( node->mName.data );
        if ( jointMapping.find( name ) != jointMapping.end() )
        {
            for ( uint32_t i = 0; i < node->mNumChildren; i++ )
            {
                std::string childName( node->mChildren[i]->mName.data );
                // PG_ASSERT( jointMapping.find( childName ) != jointMapping.end(), "AI bone has a non-bone node as a child" );
                if  ( jointMapping.find( childName ) != jointMapping.end() )
                {
                    joints[jointMapping[name]].children.push_back( jointMapping[childName] );
                }
            }
        }
        for ( uint32_t i = 0; i < node->mNumChildren; i++ )
        {
            FindJointChildren( node->mChildren[i], jointMapping, joints );
        }
    }

    static void BuildAINodeMap( aiNode* pNode, std::unordered_map< std::string, aiNode* >& nodes )
    {
        std::string name( pNode->mName.data );
        PG_ASSERT( nodes.find( name ) == nodes.end(), "Map of AI nodes already contains name '" + name + "'" );
        nodes[name] = pNode;
    
        for ( uint32_t i = 0; i < pNode->mNumChildren; i++ )
        {
            BuildAINodeMap( pNode->mChildren[i], nodes );
        }
    }

    static void BuildAIAnimNodeMap( aiNode* pNode, aiAnimation* pAnimation, std::unordered_map< std::string, aiNodeAnim* >& animNodes )
    {
        std::string nodeName( pNode->mName.data );
        aiNodeAnim* pNodeAnim = FindAINodeAnim( pAnimation, nodeName );
    
        if ( pNodeAnim )
        {
            PG_ASSERT( animNodes.find( nodeName ) == animNodes.end(), "Map of AI animation nodes already contains name '" + nodeName + "'" );
            animNodes[nodeName] = pNodeAnim;
        }
    
        for ( uint32_t i = 0; i < pNode->mNumChildren; i++ )
        {
            BuildAIAnimNodeMap( pNode->mChildren[i], pAnimation, animNodes );
        }
    }

    static std::set< float > GetAllAnimationTimes( aiAnimation* pAnimation )
    {
        std::set< float > times;
        for ( uint32_t i = 0; i < pAnimation->mNumChannels; ++i )
        {
            aiNodeAnim* p = pAnimation->mChannels[i];
        
            for ( uint32_t i = 0; i < p->mNumPositionKeys; ++i )
            {
                times.insert( static_cast< float >( p->mPositionKeys[i].mTime ) );
            }
            for ( uint32_t i = 0; i < p->mNumRotationKeys; ++i )
            {
                times.insert( static_cast< float >( p->mRotationKeys[i].mTime ) );
            }
            for ( uint32_t i = 0; i < p->mNumPositionKeys; ++i )
            {
                times.insert( static_cast< float >( p->mScalingKeys[i].mTime ) );
            }
        }

        return times;
    }

    static void AnalyzeMemoryEfficiency( aiAnimation* pAnimation, const std::set< float >& times )
    {
        double efficient = 0;
        for ( uint32_t i = 0; i < pAnimation->mNumChannels; ++i )
        {
            aiNodeAnim* p = pAnimation->mChannels[i];
            efficient += p->mNumPositionKeys * sizeof( glm::vec3 ) + p->mNumRotationKeys * sizeof( glm::quat ) + p->mNumScalingKeys * sizeof( glm::vec3 );
        }
        double easy = static_cast< double >( pAnimation->mNumChannels * times.size() * ( sizeof( glm::vec3 ) + sizeof( glm::quat ) + sizeof( glm::vec3 ) ) );

        efficient /= ( 2 << 20 );
        easy      /= ( 2 << 20 );
        LOG( "Efficient method = ", efficient, "MB, easy method = ", easy, "MB, ratio = ", easy / efficient );
    }

    static void ParseMaterials( const std::string& filename, std::shared_ptr< SkinnedModel >& model, const aiScene* scene )
    {
        std::string::size_type slashIndex = filename.find_last_of( "/" );
        std::string dir;

        if ( slashIndex == std::string::npos)
        {
            dir = ".";
        }
        else if ( slashIndex == 0 )
        {
            dir = "/";
        }
        else
        {
            dir = filename.substr( 0, slashIndex );
        }

        model->materials.resize( scene->mNumMaterials );
        for ( uint32_t mtlIdx = 0; mtlIdx < scene->mNumMaterials; ++mtlIdx )
        {
            const aiMaterial* pMaterial = scene->mMaterials[mtlIdx];
            model->materials[mtlIdx] = std::make_shared< Material >();
            aiString name;
            aiColor3D color;
            pMaterial->Get( AI_MATKEY_NAME, name );
            model->materials[mtlIdx]->name = name.C_Str();

            color = aiColor3D( 0.f, 0.f, 0.f );
            pMaterial->Get( AI_MATKEY_COLOR_AMBIENT, color );
            model->materials[mtlIdx]->Ka = { color.r, color.g, color.b };
            color = aiColor3D( 0.f, 0.f, 0.f );
            pMaterial->Get( AI_MATKEY_COLOR_DIFFUSE, color );
            model->materials[mtlIdx]->Kd = { color.r, color.g, color.b };
            color = aiColor3D( 0.f, 0.f, 0.f );
            pMaterial->Get( AI_MATKEY_COLOR_SPECULAR, color );
            model->materials[mtlIdx]->Ks = { color.r, color.g, color.b };
            color = aiColor3D( 0.f, 0.f, 0.f );
            pMaterial->Get( AI_MATKEY_COLOR_EMISSIVE, color );
            model->materials[mtlIdx]->Ke = { color.r, color.g, color.b };
            color = aiColor3D( 0.f, 0.f, 0.f );
            float Ns;
            pMaterial->Get( AI_MATKEY_SHININESS, Ns );
            model->materials[mtlIdx]->Ns = Ns;
            LOG( "Material[", mtlIdx, "].Ka = ", model->materials[mtlIdx]->Ka );
            LOG( "Material[", mtlIdx, "].Kd = ", model->materials[mtlIdx]->Kd );
            LOG( "Material[", mtlIdx, "].Ks = ", model->materials[mtlIdx]->Ks );
            LOG( "Material[", mtlIdx, "].Ke = ", model->materials[mtlIdx]->Ke );
            LOG( "Material[", mtlIdx, "].Ns = ", model->materials[mtlIdx]->Ns );

            if ( pMaterial->GetTextureCount( aiTextureType_DIFFUSE ) > 0 )
            {
                aiString path;

                if ( pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS)
                {
                    std::string p( path.data );
                
                    if ( p.substr(0, 2) == ".\\" )
                    {                    
                        p = p.substr(2, p.size() - 2);
                    }
                               
                    std::string fullPath = dir + "/" + p;
                    LOG( "Material[", mtlIdx, "] has diffuse texture '", fullPath, "'" );
                }
            }
        }
    }

    bool SkinnedModel::LoadFBX( const std::string& filename, std::shared_ptr< SkinnedModel >& model )
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile( filename.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices );
        if ( !scene )
        {
            LOG_ERR( "Error parsing FBX file '", filename.c_str(), "': ", importer.GetErrorString() );
            return false;
        }

        // model->globalInverseTransform = glm::inverse( AssimpToGlmMat4( scene->mRootNode->mTransformation ) );

        model->meshes.resize( scene->mNumMeshes );       
        uint32_t numVertices = 0;
        uint32_t numIndices = 0;
        LOG( "Num animations: ", scene->mNumAnimations );
    
        // Count the number of vertices and indices
        for ( size_t i = 0 ; i < model->meshes.size(); i++ )
        {
            model->meshes[i].materialIndex = scene->mMeshes[i]->mMaterialIndex;
            model->meshes[i].m_numIndices  = scene->mMeshes[i]->mNumFaces * 3;
            model->meshes[i].m_startVertex = numVertices;
            model->meshes[i].m_startIndex  = numIndices;
        
            numVertices += scene->mMeshes[i]->mNumVertices;
            numIndices  += model->meshes[i].m_numIndices;
        }
    
        model->vertices.reserve( numVertices );
        model->normals.reserve( numVertices );
        model->uvs.reserve( numVertices );
        model->indices.reserve( numIndices );
        model->blendWeights.resize( numVertices );
        LOG( "Num vertices = ", numVertices );
        LOG( "Num indices = ", numIndices );

        std::unordered_map< std::string, uint32_t > jointNameToIndexMap;
        
        // Initialize the meshes in the scene one by one
        Joint rootJointPlaceholder;
        rootJointPlaceholder.name = "___DUMMY_ROOT_BONE_PLACEHOLDER___";
        model->joints.push_back( rootJointPlaceholder );
        for ( size_t meshIdx = 0; meshIdx < model->meshes.size(); ++meshIdx )
        {
            const aiMesh* paiMesh = scene->mMeshes[meshIdx];
            const aiVector3D Zero3D( 0.0f, 0.0f, 0.0f );
    
            // Populate the vertex attribute vectors
            for ( uint32_t vIdx = 0; vIdx < paiMesh->mNumVertices ; ++vIdx )
            {
                const aiVector3D* pPos      = &( paiMesh->mVertices[vIdx] );
                const aiVector3D* pNormal   = &( paiMesh->mNormals[vIdx] );
                const aiVector3D* pTexCoord = paiMesh->HasTextureCoords( 0 ) ? &( paiMesh->mTextureCoords[0][vIdx] ) : &Zero3D;

                model->vertices.emplace_back( pPos->x, pPos->y, pPos->z );
                model->normals.emplace_back( pNormal->x, pNormal->y, pNormal->z );
                model->uvs.emplace_back( pTexCoord->x, pTexCoord->y );
            }

            LOG( "Mesh[", meshIdx, "] Num joints: ", paiMesh->mNumBones );
            for ( uint32_t boneIdx = 0; boneIdx < paiMesh->mNumBones; ++boneIdx )
            {
                uint32_t jointIndex = 0;
                std::string jointName( paiMesh->mBones[boneIdx]->mName.data );

                if ( jointNameToIndexMap.find( jointName ) == jointNameToIndexMap.end() )
                {
                    jointIndex = static_cast< uint32_t >( model->joints.size() );
                    Joint newJoint;
                    newJoint.name                         = jointName;
                    newJoint.boneToMeshSpaceBindTransform = AiToGLMMat4( paiMesh->mBones[boneIdx]->mOffsetMatrix );
                    model->joints.push_back( newJoint );
                    jointNameToIndexMap[jointName] = jointIndex;
                }
                else
                {
                    jointIndex = jointNameToIndexMap[jointName];
                }
        
                for ( unsigned int weightIdx = 0; weightIdx < paiMesh->mBones[boneIdx]->mNumWeights; ++weightIdx )
                {
                    uint32_t vertexID = model->meshes[meshIdx].m_startVertex + paiMesh->mBones[boneIdx]->mWeights[weightIdx].mVertexId;
                    float weight      = paiMesh->mBones[boneIdx]->mWeights[weightIdx].mWeight;
                    model->blendWeights[vertexID].AddJointData( jointIndex, weight );
                }
            }

            for ( size_t iIdx = 0 ; iIdx < paiMesh->mNumFaces ; ++iIdx )
            {
                const aiFace& face = paiMesh->mFaces[iIdx];
                PG_ASSERT( face.mNumIndices == 3 );
                model->indices.push_back( face.mIndices[0] );
                model->indices.push_back( face.mIndices[1] );
                model->indices.push_back( face.mIndices[2] );
            }
        }
        
        // Add root bone
        std::unordered_map< std::string, aiNode* > aiNodeMap;
        BuildAINodeMap( scene->mRootNode, aiNodeMap );
        for ( uint32_t joint = 1; joint < (uint32_t) model->joints.size(); ++joint )
        {
            aiNode* parent = aiNodeMap[model->joints[joint].name]->mParent;
            PG_ASSERT( parent );
            std::string parentName( parent->mName.data );
            if ( jointNameToIndexMap.find( parentName ) == jointNameToIndexMap.end() )
            {
                Joint& rootBone = model->joints[0];
                rootBone.name = parentName;
                rootBone.modelSpaceTransform = glm::mat4( 1 );
                while ( parent != NULL )
                {
                    rootBone.modelSpaceTransform = AiToGLMMat4( parent->mTransformation ) * rootBone.modelSpaceTransform;
                    parent = parent->mParent;
                }
                for ( uint32_t i = 1; i < (uint32_t) model->joints.size(); ++i )
                {
                    std::string boneParent = aiNodeMap[model->joints[i].name]->mParent->mName.data;
                    if ( rootBone.name == boneParent )
                    {
                        rootBone.children.push_back( i );
                    }
                }
                break;
            }
        }
        LOG( "Parent bone = ", model->joints[0].name );
        LOG( "Parent children: " );
        for ( const auto& child : model->joints[0].children )
        {
            LOG( "\t", model->joints[child].name );
        }

        //ReadNodeHeirarchy( scene->mRootNode, scene->mNumAnimations, scene->mAnimations );
        //LOG( "" );

        FindJointChildren( scene->mRootNode, jointNameToIndexMap, model->joints );

        model->animations.resize( scene->mNumAnimations );
        for ( uint32_t animIdx = 0; animIdx < scene->mNumAnimations; ++animIdx )
        {
            Animation& pgAnim         = model->animations[animIdx];
            aiAnimation* aiAnim       = scene->mAnimations[animIdx];
            pgAnim.duration           = static_cast< float >( aiAnim->mDuration );
            PG_ASSERT( pgAnim.duration > 0 );
            pgAnim.ticksPerSecond     = static_cast< float >( aiAnim->mTicksPerSecond );
            if ( aiAnim->mTicksPerSecond == 0 )
            {
                LOG_WARN( "Animation '", aiAnim->mName.C_Str(), "' does not specify TicksPerSecond. Using default of 30" );
                pgAnim.ticksPerSecond = 30;
            }

            LOG( "Animation '", aiAnim->mName.C_Str(), "' duration = ", pgAnim.duration, ", ticksPerSecond = ", pgAnim.ticksPerSecond );
            std::set< float > keyFrameTimes = GetAllAnimationTimes( aiAnim );
            AnalyzeMemoryEfficiency( aiAnim, keyFrameTimes );
            std::unordered_map< std::string, aiNodeAnim* > aiAnimNodeMap;
            BuildAIAnimNodeMap( scene->mRootNode, aiAnim, aiAnimNodeMap );
            PG_ASSERT( aiAnimNodeMap.size() == model->joints.size(), "Animation does not have the same skeleton as model" );

            pgAnim.keyFrames.resize( keyFrameTimes.size() );
            int frameIdx = 0;
            for ( const auto& time : keyFrameTimes )
            {
                pgAnim.keyFrames[frameIdx].time = time;
                pgAnim.keyFrames[frameIdx].jointSpaceTransforms.resize( model->joints.size() );
                for ( uint32_t joint = 0; joint < (uint32_t) model->joints.size(); ++joint )
                {
                    aiNodeAnim* animNode       = aiAnimNodeMap[model->joints[joint].name];
                    JointTransform& jTransform = pgAnim.keyFrames[frameIdx].jointSpaceTransforms[joint];
                    uint32_t i;
                    float dt;

                    for ( i = 0; i < animNode->mNumPositionKeys - 1 && time <= (float) animNode->mPositionKeys[i + 1].mTime; ++i );
                    PG_ASSERT( i != animNode->mNumPositionKeys, "Time is greater than all position keyframes for this bone" );
                    glm::vec3 currentPos = AiToGLMVec3( animNode->mPositionKeys[i].mValue );
                    glm::vec3 nextPos    = AiToGLMVec3( animNode->mPositionKeys[i + 1].mValue );
                    dt                   = ( time - (float) animNode->mPositionKeys[i].mTime ) / ( (float) animNode->mPositionKeys[i + 1].mTime - (float) animNode->mPositionKeys[i + 1].mTime );
                    jTransform.position  = currentPos + dt * ( nextPos - currentPos );

                    for ( i = 0; i < animNode->mNumRotationKeys - 1 && time <= (float) animNode->mRotationKeys[i + 1].mTime; ++i );
                    PG_ASSERT( i != animNode->mNumRotationKeys, "Time is greater than all position keyframes for this bone" );
                    glm::quat currentRot = AiToGLMQuat( animNode->mRotationKeys[i].mValue );
                    glm::quat nextRot    = AiToGLMQuat( animNode->mRotationKeys[i + 1].mValue );
                    dt                   = ( time - (float) animNode->mRotationKeys[i].mTime ) / ( (float) animNode->mRotationKeys[i + 1].mTime - (float) animNode->mRotationKeys[i + 1].mTime );
                    jTransform.rotation  = glm::normalize( glm::mix( currentRot, nextRot, dt ) );

                    for ( i = 0; i < animNode->mNumScalingKeys - 1 && time <= (float) animNode->mScalingKeys[i + 1].mTime; ++i );
                    PG_ASSERT( i != animNode->mNumScalingKeys, "Time is greater than all position keyframes for this bone" );
                    glm::vec3 currentScale = AiToGLMVec3( animNode->mScalingKeys[i].mValue );
                    glm::vec3 nextScale    = AiToGLMVec3( animNode->mScalingKeys[i + 1].mValue );
                    dt                     = ( time - (float) animNode->mScalingKeys[i].mTime ) / ( (float) animNode->mScalingKeys[i + 1].mTime - (float) animNode->mScalingKeys[i + 1].mTime );
                    jTransform.scale       = currentPos + dt * ( nextPos - currentPos );
                }
            }
        }

        ParseMaterials( filename, model, scene );

        LOG( "Num joints: ", model->joints.size() );
        LOG( "Num blendWeights: ", model->blendWeights.size() );
        for ( size_t meshIdx = 0; meshIdx < model->meshes.size(); ++meshIdx )
        {
            LOG( "Mesh[", meshIdx, "].m_startIndex = ", model->meshes[meshIdx].m_startIndex );
            LOG( "Mesh[", meshIdx, "].m_startVertex = ", model->meshes[meshIdx].m_startVertex );
            LOG( "Mesh[", meshIdx, "].m_numIndices = ", model->meshes[meshIdx].m_numIndices );
        }

        for ( size_t i = 0; i < model->blendWeights.size(); ++i )
        {
            auto& data = model->blendWeights[i];
            float sum = data.weights[0] + data.weights[1] + data.weights[2] + data.weights[3];
            data.weights /= sum;
            // LOG( "Bone Vertex[", i, "]: joints = ", data.joints.x, " ", data.joints.y, " ", data.joints.z, " ", data.joints.w, ", weights = ", data.weights );
        }
        model->RecalculateAABB();
        model->UploadToGpu();

        return true;
    }

    uint32_t SkinnedModel::GetNumVertices() const
    {
        return m_numVertices;
    }

    uint32_t SkinnedModel::GetVertexOffset() const
    {
        return 0;
    }

    uint32_t SkinnedModel::GetNormalOffset() const
    {
        return m_normalOffset;
    }

    uint32_t SkinnedModel::GetUVOffset() const
    {
        return m_uvOffset;
    }

    uint32_t SkinnedModel::GetBlendWeightOffset() const
    {
        return m_blendWeightOffset;
    }

    Gfx::IndexType SkinnedModel::GetIndexType() const
    {
        return Gfx::IndexType::UNSIGNED_INT;
    }

} // namespace Progression