#include "resource/model.hpp"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "core/assert.hpp"
#include "core/math.hpp"
#include "core/time.hpp"
#include "graphics/debug_marker.hpp"
#include "graphics/vulkan.hpp"
#include "meshoptimizer/src/meshoptimizer.h"
#include "resource/resource_manager.hpp"
#include "utils/logger.hpp"
#include "utils/serialize.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"
#include <filesystem>
#include <set>

#define PRINT_OPTIMIZATION_ANALYSIS NOT_IN_USE

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
    return glm::transpose( tmp );
}

static glm::vec3 AiToGLMVec3( const aiVector3D& v )
{
    return { v.x, v.y, v.z };
}

static glm::quat AiToGLMQuat( const aiQuaternion& q )
{
    return { q.w, q.x, q.y, q.z };
}

static std::string TrimWhiteSpace( const std::string& s )
{
    size_t start = s.find_first_not_of( " \t" );
    size_t end   = s.find_last_not_of( " \t" );
    return s.substr( start, end - start + 1 );
}

class Vertex
{
public:
    Vertex() = default;

    bool operator==( const Vertex& other ) const
    {
        return vertex == other.vertex && normal == other.normal && uv == other.uv &&
               blendWeight.weights == other.blendWeight.weights && blendWeight.joints == other.blendWeight.joints;
    }

    glm::vec3 vertex = glm::vec3( 0 );
    glm::vec3 normal = glm::vec3( 0 );
    glm::vec2 uv     = glm::vec2( 0 );
    Progression::BlendWeight blendWeight;
};

namespace Progression
{

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

    void Model::ApplyPoseToJoints( uint32_t jointIdx, const glm::mat4& parentTransform, std::vector< glm::mat4 >& transformBuffer )
    {
        glm::mat4 currentTransform = parentTransform * transformBuffer[jointIdx];
        for ( const auto& child : skeleton.joints[jointIdx].children )
        {
            ApplyPoseToJoints( child, currentTransform, transformBuffer );
        }
        transformBuffer[jointIdx] = currentTransform * skeleton.joints[jointIdx].inverseBindTransform;
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

    static std::shared_ptr< Image > LoadAssimpTexture( const aiMaterial* pMaterial, aiTextureType texType )
    {
        namespace fs = std::filesystem;
        aiString path;
        if ( pMaterial->GetTexture( texType, 0, &path, NULL, NULL, NULL, NULL, NULL ) == AI_SUCCESS )
        {
            std::string name = TrimWhiteSpace( path.data );
            if ( ResourceManager::Get< Image >( name ) )
            {
                return ResourceManager::Get< Image >( name );
            }

            std::string fullPath;
            // search for texture starting with
            if ( fs::exists( PG_RESOURCE_DIR + name ) )
            {
                fullPath = PG_RESOURCE_DIR + name;
            }
            else
            {
                std::string basename = fs::path( name ).filename().string();
                for( auto itEntry = fs::recursive_directory_iterator( PG_RESOURCE_DIR ); itEntry != fs::recursive_directory_iterator(); ++itEntry )
                {
                    if ( basename == itEntry->path().filename().string() )
                    {
                        fullPath = fs::absolute( itEntry->path() ).string();
                        break;
                    }
                }
            }
                    
            if ( fullPath != "" )
            {
                auto ret = Image::Load2DImageWithDefaultSettings( fullPath );
                if ( !ret )
                {
                    LOG_ERR( "Failed to load texture '", name, "' with default settings" );
                    return false;
                }
                return ret;
            }
            else
            {
                LOG_ERR( "Could not find file '", name, "'" );
            }
        }
        else
        {
            LOG_ERR( "Could not get texture of type: ", texType );
        }

        return nullptr;
    }

    static bool ParseMaterials( const std::string& filename, Model* model, const aiScene* scene )
    {
        namespace fs = std::filesystem;
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

            if ( pMaterial->GetTextureCount( aiTextureType_DIFFUSE ) > 0 )
            {
                PG_ASSERT( pMaterial->GetTextureCount( aiTextureType_DIFFUSE ) == 1, "Can't have more than 1 diffuse texture per material" );
                model->materials[mtlIdx]->map_Kd = LoadAssimpTexture( pMaterial, aiTextureType_DIFFUSE );
                if ( !model->materials[mtlIdx]->map_Kd )
                {
                    return false;
                }
            }
            // Assimp doesnt seem to support the OBJ PBR extension for actual normal maps, so use
            // "map_bump" instead which turns into aiTextureType_HEIGHT
            PG_ASSERT( pMaterial->GetTextureCount( aiTextureType_NORMALS ) == 0 );
            if ( pMaterial->GetTextureCount( aiTextureType_HEIGHT ) > 0 )
            {
                PG_ASSERT( pMaterial->GetTextureCount( aiTextureType_HEIGHT ) == 1, "Can't have more than 1 normal map per material" );
                model->materials[mtlIdx]->map_Norm = LoadAssimpTexture( pMaterial, aiTextureType_HEIGHT );
                if ( !model->materials[mtlIdx]->map_Norm )
                {
                    return false;
                }
            }
        }

        return true;
    }

    Model::~Model()
    {
        if ( vertexBuffer )
        {
            vertexBuffer.Free();
        }
        if ( indexBuffer )
        {
            indexBuffer.Free();
        }
    }

    bool Model::Load( ResourceCreateInfo* baseInfo )
    {
        PG_ASSERT( baseInfo );
        ModelCreateInfo* createInfo = static_cast< ModelCreateInfo* >( baseInfo );
        name = createInfo->name;
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile( createInfo->filename.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace );
        if ( !scene )
        {
            LOG_ERR( "Error parsing FBX file '", createInfo->filename.c_str(), "': ", importer.GetErrorString() );
            return false;
        }

        meshes.resize( scene->mNumMeshes );       
        uint32_t numVertices = 0;
        uint32_t numIndices = 0;    
        for ( size_t i = 0 ; i < meshes.size(); i++ )
        {
            meshes[i].name          = scene->mMeshes[i]->mName.C_Str();
            meshes[i].materialIndex = scene->mMeshes[i]->mMaterialIndex;
            meshes[i].numIndices    = scene->mMeshes[i]->mNumFaces * 3;
            meshes[i].numVertices   = scene->mMeshes[i]->mNumVertices;
            meshes[i].startVertex   = numVertices;
            meshes[i].startIndex    = numIndices;
        
            numVertices += scene->mMeshes[i]->mNumVertices;
            numIndices  += meshes[i].numIndices;
        }
        vertices.reserve( numVertices );
        normals.reserve( numVertices );
        uvs.reserve( numVertices );
        indices.reserve( numIndices );
        blendWeights.resize( numVertices );

        std::unordered_map< std::string, uint32_t > jointNameToIndexMap;
        
        std::unordered_map< std::string, aiNode* > aiNodeMap;
        BuildAINodeMap( scene->mRootNode, aiNodeMap );
        // Assimp doesn't seem to provide the root bone in the actual bone list, but animations are provided the scene
        // node that would be the root bone. Store a placeholder for it now, so that spot [0] is reserved, and find
        // the actual root bone after parsing the other bones.
        Joint rootJointPlaceholder;
        rootJointPlaceholder.name = "___ROOT_BONE_PLACEHOLDER___";
        skeleton.joints.push_back( rootJointPlaceholder );
        for ( size_t meshIdx = 0; meshIdx < meshes.size(); ++meshIdx )
        {
            const aiMesh* paiMesh = scene->mMeshes[meshIdx];
            const aiVector3D Zero3D( 0.0f, 0.0f, 0.0f );

            for ( uint32_t vIdx = 0; vIdx < paiMesh->mNumVertices ; ++vIdx )
            {
                const aiVector3D* pPos    = &paiMesh->mVertices[vIdx];
                const aiVector3D* pNormal = &paiMesh->mNormals[vIdx];
                vertices.emplace_back( pPos->x, pPos->y, pPos->z );
                normals.emplace_back( pNormal->x, pNormal->y, pNormal->z );

                if ( paiMesh->HasTextureCoords( 0 ) )
                {
                    const aiVector3D* pTexCoord = &paiMesh->mTextureCoords[0][vIdx];
                    uvs.emplace_back( pTexCoord->x, pTexCoord->y );
                }
                if ( paiMesh->HasTangentsAndBitangents() )
                {
                    const aiVector3D* pTangent = &paiMesh->mTangents[vIdx];
                    glm::vec3 t( pTangent->x, pTangent->y, pTangent->z );
                    // const glm::vec3& n = normals[vIdx];
                    // t = glm::normalize( t - n * glm::dot( n, t ) ); // does assimp orthogonalize the tangents automatically?
                    tangents.emplace_back( t );
                }
            }

            for ( uint32_t boneIdx = 0; boneIdx < paiMesh->mNumBones; ++boneIdx )
            {
                uint32_t jointIndex = 0;
                std::string jointName( paiMesh->mBones[boneIdx]->mName.data );

                if ( jointNameToIndexMap.find( jointName ) == jointNameToIndexMap.end() )
                {
                    jointIndex = static_cast< uint32_t >( skeleton.joints.size() );
                    Joint newJoint;
                    newJoint.name                 = jointName;
                    newJoint.inverseBindTransform = AiToGLMMat4( paiMesh->mBones[boneIdx]->mOffsetMatrix );
                    skeleton.joints.push_back( newJoint );
                    jointNameToIndexMap[jointName] = jointIndex;
                }
                else
                {
                    jointIndex = jointNameToIndexMap[jointName];
                }
        
                for ( unsigned int weightIdx = 0; weightIdx < paiMesh->mBones[boneIdx]->mNumWeights; ++weightIdx )
                {
                    uint32_t vertexID = meshes[meshIdx].startVertex + paiMesh->mBones[boneIdx]->mWeights[weightIdx].mVertexId;
                    float weight      = paiMesh->mBones[boneIdx]->mWeights[weightIdx].mWeight;
                    blendWeights[vertexID].AddJointData( jointIndex, weight );
                }
            }

            for ( size_t iIdx = 0; iIdx < paiMesh->mNumFaces; ++iIdx )
            {
                const aiFace& face = paiMesh->mFaces[iIdx];
                PG_ASSERT( face.mNumIndices == 3 );
                indices.push_back( face.mIndices[0] );
                indices.push_back( face.mIndices[1] );
                indices.push_back( face.mIndices[2] );
            }
        }

        // Find actual root bone and children if there is a skeleton
        if ( skeleton.joints.size() == 1 )
        {
            skeleton.joints.clear();
        }
        else
        {
            for ( uint32_t joint = 1; joint < (uint32_t) skeleton.joints.size(); ++joint )
            {
                aiNode* parent = aiNodeMap[skeleton.joints[joint].name]->mParent;
                PG_ASSERT( parent );
                std::string parentName( parent->mName.data );
                if ( jointNameToIndexMap.find( parentName ) == jointNameToIndexMap.end() )
                {
                    Joint& rootBone = skeleton.joints[0];
                    rootBone.name = parentName;
                    rootBone.inverseBindTransform = glm::mat4( 1 );
                    while ( parent != NULL )
                    {
                        rootBone.inverseBindTransform = AiToGLMMat4( parent->mTransformation ) * rootBone.inverseBindTransform;
                        parent = parent->mParent;
                    }
                    for ( uint32_t i = 1; i < (uint32_t) skeleton.joints.size(); ++i )
                    {
                        std::string boneParent = aiNodeMap[skeleton.joints[i].name]->mParent->mName.data;
                        if ( rootBone.name == boneParent )
                        {
                            rootBone.children.push_back( i );
                        }
                    }
                    rootBone.inverseBindTransform = glm::inverse( rootBone.inverseBindTransform );
                    break;
                }
            }
            FindJointChildren( scene->mRootNode, jointNameToIndexMap, skeleton.joints );
            jointNameToIndexMap[skeleton.joints[0].name] = 0;
        }

        //ReadNodeHeirarchy( scene->mRootNode, scene->mNumAnimations, scene->mAnimations );
        //LOG( "" );

        animations.resize( scene->mNumAnimations );
        for ( uint32_t animIdx = 0; animIdx < scene->mNumAnimations; ++animIdx )
        {
            Animation& pgAnim         = animations[animIdx];
            aiAnimation* aiAnim       = scene->mAnimations[animIdx];
            pgAnim.name               = aiAnim->mName.data;
            pgAnim.duration           = static_cast< float >( aiAnim->mDuration );
            PG_ASSERT( pgAnim.duration > 0 );
            pgAnim.ticksPerSecond     = static_cast< float >( aiAnim->mTicksPerSecond );
            if ( aiAnim->mTicksPerSecond == 0 )
            {
                LOG_WARN( "Animation '", aiAnim->mName.C_Str(), "' does not specify TicksPerSecond. Using default of 30" );
                pgAnim.ticksPerSecond = 30;
            }

            std::set< float > keyFrameTimes = GetAllAnimationTimes( aiAnim );
            // AnalyzeMemoryEfficiency( aiAnim, keyFrameTimes );
            std::unordered_map< std::string, aiNodeAnim* > aiAnimNodeMap;
            BuildAIAnimNodeMap( scene->mRootNode, aiAnim, aiAnimNodeMap );
            PG_ASSERT( aiAnimNodeMap.size() == skeleton.joints.size(), "Animation does not have the same skeleton as model" );

            pgAnim.keyFrames.resize( keyFrameTimes.size() );
            int frameIdx = 0;
            for ( const auto& time : keyFrameTimes )
            {
                pgAnim.keyFrames[frameIdx].time = time;
                pgAnim.keyFrames[frameIdx].jointSpaceTransforms.resize( skeleton.joints.size() );
                for ( uint32_t joint = 0; joint < (uint32_t) skeleton.joints.size(); ++joint )
                {
                    aiNodeAnim* animNode       = aiAnimNodeMap[skeleton.joints[joint].name];
                    JointTransform& jTransform = pgAnim.keyFrames[frameIdx].jointSpaceTransforms[joint];
                    uint32_t i;
                    float dt;

                    for ( i = 0; i < animNode->mNumPositionKeys - 1 && time > (float) animNode->mPositionKeys[i + 1].mTime; ++i );
                    if ( i + 1 == animNode->mNumPositionKeys )
                    {
                        jTransform.position = AiToGLMVec3( animNode->mPositionKeys[i].mValue );
                    }
                    else
                    {
                        glm::vec3 currentPos = AiToGLMVec3( animNode->mPositionKeys[i].mValue );
                        glm::vec3 nextPos    = AiToGLMVec3( animNode->mPositionKeys[i + 1].mValue );
                        dt                   = ( time - (float) animNode->mPositionKeys[i].mTime ) / ( (float) animNode->mPositionKeys[i + 1].mTime - (float) animNode->mPositionKeys[i].mTime );
                        jTransform.position  = currentPos + dt * ( nextPos - currentPos );
                    }

                    for ( i = 0; i < animNode->mNumRotationKeys - 1 && time > (float) animNode->mRotationKeys[i + 1].mTime; ++i );
                    if ( i + 1 == animNode->mNumRotationKeys )
                    {
                        jTransform.rotation = AiToGLMQuat( animNode->mRotationKeys[i].mValue );
                    }
                    else
                    {
                        glm::quat currentRot = glm::normalize( AiToGLMQuat( animNode->mRotationKeys[i].mValue ) );
                        glm::quat nextRot    = glm::normalize( AiToGLMQuat( animNode->mRotationKeys[i + 1].mValue ) );
                        dt                   = ( time - (float) animNode->mRotationKeys[i].mTime ) / ( (float) animNode->mRotationKeys[i + 1].mTime - (float) animNode->mRotationKeys[i].mTime );
                        jTransform.rotation  = glm::normalize( glm::slerp( currentRot, nextRot, dt ) );
                    }
                    

                    for ( i = 0; i < animNode->mNumScalingKeys - 1 && time > (float) animNode->mScalingKeys[i + 1].mTime; ++i );
                    if ( i + 1 == animNode->mNumScalingKeys )
                    {
                        jTransform.scale = AiToGLMVec3( animNode->mScalingKeys[i].mValue );
                    }
                    else
                    {
                        glm::vec3 currentScale = AiToGLMVec3( animNode->mScalingKeys[i].mValue );
                        glm::vec3 nextScale    = AiToGLMVec3( animNode->mScalingKeys[i + 1].mValue );
                        dt                     = ( time - (float) animNode->mScalingKeys[i].mTime ) / ( (float) animNode->mScalingKeys[i + 1].mTime - (float) animNode->mScalingKeys[i].mTime );
                        jTransform.scale       = currentScale + dt * ( nextScale - nextScale );
                    }
                    
                }
                ++frameIdx;
            }
        }

        // renormalize the blend weights to 1 if skeleton exists
        if ( skeleton.joints.size() )
        {
            for ( size_t i = 0; i < blendWeights.size(); ++i )
            {
                auto& data = blendWeights[i];
                float sum = data.weights[0] + data.weights[1] + data.weights[2] + data.weights[3];
                PG_ASSERT( sum > 0 );
                data.weights /= sum;
            }
        }
        else
        {
            blendWeights = std::vector< BlendWeight >{};
        }

        RecalculateAABB();

        if ( createInfo->optimize )
        {
            Optimize();
        }
        if ( createInfo->createGpuCopy )
        {
            UploadToGpu();
        }
        if ( createInfo->freeCpuCopy )
        {
            FreeGeometry();
        }

        if ( !ParseMaterials( createInfo->filename, this, scene ) )
        {
            LOG_ERR( "Could not load the model's materials" );
            return false;
        }
        // materials that were not already in the resource manager don't have their CPU copies freed by default
        if ( createInfo->freeCpuCopy )
        {
            for ( const auto& mat : materials )
            {
                if ( mat->map_Kd && !ResourceManager::Get< Image >( mat->map_Kd->name ) )
                {
                    mat->map_Kd->FreeCpuCopy();
                }
            }
        }

        return true;
    }

    void Model::Move( std::shared_ptr< Resource > dst )
    {
        PG_ASSERT( std::dynamic_pointer_cast< Model >( dst ) );
        Model* dstPtr = (Model*) dst.get();
        *dstPtr              = std::move( *this );
    }

    bool Model::Serialize( std::ofstream& out ) const
    {
        uint32_t numMaterials = static_cast< uint32_t >( materials.size() );
        serialize::Write( out, numMaterials );
        for ( uint32_t i = 0; i < numMaterials; ++i )
        {
            if ( !materials[i]->Serialize( out ) )
            {
                LOG( "Could not write material: ", i, ", of model: ", name, " to fastfile" );
                return false;
            }
        }
        uint32_t numVertices     = static_cast< uint32_t >( vertices.size() );
        uint32_t numUVs          = static_cast< uint32_t >( uvs.size() );
        uint32_t numBlendWeights = static_cast< uint32_t >( blendWeights.size() );
        uint32_t numTangents     = static_cast< uint32_t >( tangents.size() );
        uint32_t numIndices      = static_cast< uint32_t >( indices.size() );
        serialize::Write( out, numVertices );
        serialize::Write( out, numUVs );
        serialize::Write( out, numBlendWeights );
        serialize::Write( out, numTangents );
        serialize::Write( out, numIndices );
        serialize::Write( out, (char*) vertices.data(),     numVertices * sizeof( glm::vec3 ) );
        serialize::Write( out, (char*) normals.data(),      numVertices * sizeof( glm::vec3 ) );
        serialize::Write( out, (char*) uvs.data(),          numUVs * sizeof( glm::vec2 ) );
        serialize::Write( out, (char*) blendWeights.data(), numBlendWeights * 2 * sizeof( glm::vec4 ) );
        serialize::Write( out, (char*) tangents.data(),     numTangents * sizeof( glm::vec3 ) );
        serialize::Write( out, (char*) indices.data(),      numIndices * sizeof( uint32_t ) );

        serialize::Write( out, aabb.min );
        serialize::Write( out, aabb.max );
        serialize::Write( out, aabb.extent );
        uint32_t numMeshes = static_cast< uint32_t >( meshes.size() );
        serialize::Write( out, numMeshes );
        for ( const auto& mesh : meshes )
        {
            serialize::Write( out, mesh.name );
            serialize::Write( out, mesh.materialIndex );
            serialize::Write( out, mesh.startIndex );
            serialize::Write( out, mesh.numIndices );
            serialize::Write( out, mesh.startVertex );
            serialize::Write( out, mesh.numVertices );
        }
        skeleton.Serialize( out );
        size_t numAnimations = animations.size();
        serialize::Write( out, numAnimations );
        for ( const auto& anim : animations )
        {
            serialize::Write( out, anim.name );
            serialize::Write( out, anim.duration );
            serialize::Write( out, anim.ticksPerSecond );
            serialize::Write( out, anim.keyFrames.size() );
            for ( const auto& keyFrame : anim.keyFrames )
            {
                serialize::Write( out, keyFrame.time );
                serialize::Write( out, keyFrame.jointSpaceTransforms );
            }
        }

        return !out.fail();
    }

    bool Model::Deserialize( char*& buffer )
    {
        serialize::Read( buffer, name );
        bool freeCpuCopy;
        bool createGpuCopy;
        serialize::Read( buffer, freeCpuCopy );
        serialize::Read( buffer, createGpuCopy );

        uint32_t numMaterials;
        serialize::Read( buffer, numMaterials );
        materials.resize( numMaterials );
        for ( uint32_t i = 0; i < numMaterials; ++i )
        {
            materials[i] = std::make_shared< Material >();
            materials[i]->Deserialize( buffer );
        }

        uint32_t numVertices, numUVs, numBlendWeights, numTangents, numIndices;
        serialize::Read( buffer, numVertices );
        serialize::Read( buffer, numUVs );
        serialize::Read( buffer, numBlendWeights );
        serialize::Read( buffer, numTangents );
        serialize::Read( buffer, numIndices );
        if ( freeCpuCopy )
        {
            using namespace Progression::Gfx;
            size_t totalVertexSize = 0;
            totalVertexSize += 2 * numVertices * sizeof( glm::vec3 );
            totalVertexSize += numUVs * sizeof( glm::vec2 );
            totalVertexSize += numBlendWeights * 2 * sizeof( glm::vec4 );
            totalVertexSize += numTangents * sizeof( glm::vec3 );
            vertexBuffer = Gfx::g_renderState.device.NewBuffer( totalVertexSize, buffer, BUFFER_TYPE_VERTEX, MEMORY_TYPE_DEVICE_LOCAL, name + " VBO" );
            buffer += totalVertexSize;
            indexBuffer  = Gfx::g_renderState.device.NewBuffer( numIndices * sizeof( uint32_t ), buffer, BUFFER_TYPE_INDEX, MEMORY_TYPE_DEVICE_LOCAL, name + " IBO" );
            buffer += numIndices * sizeof( uint32_t );

            m_numVertices       = numVertices;
            m_normalOffset      = m_numVertices * sizeof( glm::vec3 );
            uint32_t offset     = m_normalOffset + numVertices * sizeof( glm::vec3 );
            if ( numUVs > 0 )
            {
                m_uvOffset = offset;
                offset += numUVs * sizeof( glm::vec2 );
            }
            if ( numBlendWeights > 0 )
            {
                m_blendWeightOffset = offset;
                offset += numBlendWeights * 2 * sizeof( glm::vec4 );
            }
            if ( numTangents )
            {
                m_tangentOffset = offset;
            }
        }
        else
        {
            vertices.resize( numVertices );
            normals.resize( numVertices );
            uvs.resize( numUVs );
            tangents.resize( numTangents );
            blendWeights.resize( numBlendWeights );
            indices.resize( numIndices );

            serialize::Read( buffer, vertices );
            serialize::Read( buffer, normals );
            serialize::Read( buffer, uvs );
            serialize::Read( buffer, blendWeights );
            serialize::Read( buffer, indices );

            if ( createGpuCopy )
            {
                UploadToGpu();
            }
        }

        serialize::Read( buffer, aabb.min );
        serialize::Read( buffer, aabb.max );
        serialize::Read( buffer, aabb.extent );
        uint32_t numMeshes;
        serialize::Read( buffer, numMeshes );
        meshes.resize( numMeshes );
        for ( auto& mesh : meshes )
        {
            serialize::Read( buffer, mesh.name );
            serialize::Read( buffer, mesh.materialIndex );
            serialize::Read( buffer, mesh.startIndex );
            serialize::Read( buffer, mesh.numIndices );
            serialize::Read( buffer, mesh.startVertex );
            serialize::Read( buffer, mesh.numVertices );
        }
        skeleton.Deserialize( buffer );

        size_t numAnimations;
        serialize::Read( buffer, numAnimations );
        animations.resize( numAnimations );
        for ( auto& anim : animations )
        {
            serialize::Read( buffer, anim.name );
            serialize::Read( buffer, anim.duration );
            serialize::Read( buffer, anim.ticksPerSecond );
            size_t numKeyFrames;
            serialize::Read( buffer, numKeyFrames );
            anim.keyFrames.resize( numKeyFrames );
            for ( auto& keyFrame : anim.keyFrames )
            {
                serialize::Read( buffer, keyFrame.time );
                serialize::Read( buffer, keyFrame.jointSpaceTransforms );
            }
        }

        return true;
    }

    void Model::RecalculateNormals()
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

    void Model::RecalculateAABB()
    {
        if ( vertices.empty() )
        {
            aabb.min = aabb.max = glm::vec3( 0 );
            return;
        }

        aabb.min = vertices[0];
        aabb.max = vertices[0];
        for ( const auto& vertex : vertices )
        {
            aabb.min = glm::min( aabb.min, vertex );
            aabb.max = glm::max( aabb.max, vertex );
        }
    }

    void Model::UploadToGpu()
    {
        using namespace Gfx;

        if ( vertexBuffer )
        {
            vertexBuffer.Free();
        }
        if ( indexBuffer )
        {
            indexBuffer.Free();
        }
        std::vector< float > vertexData( 3 * vertices.size() + 3 * normals.size() + 2 * uvs.size() + 8 * blendWeights.size() + 3 * tangents.size() );
        char* dst = (char*) vertexData.data();
        memcpy( dst, vertices.data(), vertices.size() * sizeof( glm::vec3 ) );
        dst += vertices.size() * sizeof( glm::vec3 );
        memcpy( dst, normals.data(), normals.size() * sizeof( glm::vec3 ) );
        dst += normals.size() * sizeof( glm::vec3 );
        memcpy( dst, uvs.data(), uvs.size() * sizeof( glm::vec2 ) );
        dst += uvs.size() * sizeof( glm::vec2 );
        memcpy( dst, blendWeights.data(), blendWeights.size() * 2 * sizeof( glm::vec4 ) );
        dst += blendWeights.size() * 2 * sizeof( glm::vec4 );
        memcpy( dst, blendWeights.data(), tangents.size() * sizeof( glm::vec3 ) );
        vertexBuffer = Gfx::g_renderState.device.NewBuffer( vertexData.size() * sizeof( float ), vertexData.data(), BUFFER_TYPE_VERTEX, MEMORY_TYPE_DEVICE_LOCAL, name + " VBO" );
        indexBuffer  = Gfx::g_renderState.device.NewBuffer( indices.size() * sizeof ( uint32_t ), indices.data(), BUFFER_TYPE_INDEX, MEMORY_TYPE_DEVICE_LOCAL, name + " IBO" );

        m_numVertices       = static_cast< uint32_t >( vertices.size() );
        m_normalOffset      = m_numVertices * sizeof( glm::vec3 );
        uint32_t offset     = m_normalOffset + m_numVertices * sizeof( glm::vec3 );
        if ( !uvs.empty() )
        {
            m_uvOffset = offset;
            offset += static_cast< uint32_t >( uvs.size() * sizeof( glm::vec2 ) );
        }
        if ( !blendWeights.empty() )
        {
            m_blendWeightOffset = offset;
            offset += static_cast< uint32_t >( blendWeights.size() * sizeof( glm::vec2 ) );
        }
        if ( !tangents.empty() )
        {
            m_tangentOffset = offset;
        }
    }

    void Model::FreeGeometry( bool cpuCopy, bool gpuCopy )
    {
        if ( cpuCopy )
        {
            m_numVertices = static_cast< uint32_t >( vertices.size() );
            vertices      = std::vector< glm::vec3 >();
            normals       = std::vector< glm::vec3 >();
            uvs           = std::vector< glm::vec2 >();
            indices       = std::vector< uint32_t >();
            blendWeights  = std::vector< BlendWeight >();
        }

        if ( gpuCopy )
        {
            if ( vertexBuffer )
            {
                vertexBuffer.Free();
            }
            if ( indexBuffer )
            {
                indexBuffer.Free();
            }
            m_numVertices = 0;
            m_normalOffset = m_uvOffset = m_blendWeightOffset = ~0u;
        }
    }

    void Model::Optimize()
    {
        if ( vertices.size() == 0 )
        {
            LOG_ERR( "Trying to optimize a mesh with no vertices. Did you free them after uploading to the GPU?" );
            return;
        }

        std::vector< Vertex > interleavedVerts;
        interleavedVerts.reserve( vertices.size() );
        bool hasNormals      = !normals.empty();
        bool hasUVs          = !uvs.empty();
        bool hasBlendWeights = !blendWeights.empty();
        for ( size_t i = 0; i < vertices.size(); ++i )
        {
            Vertex v;
            v.vertex = vertices[i];
            if ( hasNormals )
            {
                v.normal = normals[i];
            }
            if ( hasUVs )
            {
                v.uv = uvs[i];
            }
            if ( hasBlendWeights )
            {
                v.blendWeight = blendWeights[i];
            }

            interleavedVerts.push_back( v );
        }
        vertices.clear();
        normals.clear();
        uvs.clear();
        blendWeights.clear();

        for ( auto& mesh : meshes )
        {
            std::vector< Vertex > optVertices;
            for ( size_t idx = 0; idx < mesh.numVertices; ++idx )
            {
                optVertices.push_back( interleavedVerts[mesh.startVertex + idx] );
            }

#if USING( PRINT_OPTIMIZATION_ANALYSIS )
            LOG( "Mesh: numIndices = ", mesh.numIndices, ", numVerts = ", mesh.numVertices );
            const size_t kCacheSize = 16;
            meshopt_VertexCacheStatistics vcs = meshopt_analyzeVertexCache( &indices[mesh.startIndex], mesh.numIndices, optVertices.size(), kCacheSize, 0, 0 );
            meshopt_OverdrawStatistics os     = meshopt_analyzeOverdraw( &indices[mesh.startIndex], mesh.numIndices, &optVertices[0].vertex.x, optVertices.size(), sizeof( Vertex ) );
            meshopt_VertexFetchStatistics vfs = meshopt_analyzeVertexFetch( &indices[mesh.startIndex], mesh.numIndices, optVertices.size(), sizeof( Vertex ) );
            LOG( "Before:" );
            LOG( "Average cache miss ratio (0.5 - 3.0): ", vcs.acmr );
            LOG( "Average transformed vertex ratio (1.0+): ", vcs.atvr );
            LOG( "Average overdraw (1.0+): ", os.overdraw );
            LOG( "Average overfetch ratio (1.0+): ", vfs.overfetch );
#endif // #if USING( PRINT_OPTIMIZATION_ANALYSIS )

            // vertex cache optimization should go first as it provides starting order for overdraw
            meshopt_optimizeVertexCache( &indices[mesh.startIndex], &indices[mesh.startIndex], mesh.numIndices, optVertices.size() );

            // reorder indices for overdraw, balancing overdraw and vertex cache efficiency
            const float kThreshold = 1.01f; // allow up to 1% worse ACMR to get more reordering opportunities for overdraw
            meshopt_optimizeOverdraw( &indices[mesh.startIndex], &indices[mesh.startIndex], mesh.numIndices, &optVertices[0].vertex.x,
                                      optVertices.size(), sizeof( Vertex ), kThreshold );

            // vertex fetch optimization should go last as it depends on the final index order
            meshopt_optimizeVertexFetch( &optVertices[0].vertex.x, &indices[mesh.startIndex], mesh.numIndices,
                                         &optVertices[0].vertex.x, optVertices.size(), sizeof( Vertex ) );

#if USING( PRINT_OPTIMIZATION_ANALYSIS )
            vcs = meshopt_analyzeVertexCache( &indices[mesh.startIndex], mesh.numIndices, optVertices.size(), kCacheSize, 0, 0 );
            os  = meshopt_analyzeOverdraw( &indices[mesh.startIndex], mesh.numIndices, &optVertices[0].vertex.x, optVertices.size(), sizeof( Vertex ) );
            vfs = meshopt_analyzeVertexFetch( &indices[mesh.startIndex], mesh.numIndices, optVertices.size(), sizeof( Vertex ) );
            LOG( "After:" );
            LOG( "Average cache miss ratio (0.5 - 3.0): ", vcs.acmr );
            LOG( "Average transformed vertex ratio (1.0+): ", vcs.atvr );
            LOG( "Average overdraw (1.0+): ", os.overdraw );
            LOG( "Average overfetch ratio (1.0+): ", vfs.overfetch );
#endif // #if USING( PRINT_OPTIMIZATION_ANALYSIS )

            for ( size_t i = 0; i < optVertices.size(); ++i )
            {
                const auto& v = optVertices[i];
                vertices.emplace_back( v.vertex );
                if ( hasNormals )
                {
                    normals.emplace_back( v.normal );
                }
                if ( hasUVs )
                {
                    uvs.emplace_back( v.uv );
                }
                if ( hasBlendWeights )
                {
                    auto data = v.blendWeight;
                    float sum = data.weights[0] + data.weights[1] + data.weights[2] + data.weights[3];
                    PG_ASSERT( sum > 0 );
                    data.weights /= sum;
                    blendWeights.emplace_back( data );
                }
            }
        }
    }

    uint32_t Model::GetNumVertices() const
    {
        return m_numVertices;
    }

    uint32_t Model::GetVertexOffset() const
    {
        return 0;
    }

    uint32_t Model::GetNormalOffset() const
    {
        return m_normalOffset;
    }

    uint32_t Model::GetUVOffset() const
    {
        return m_uvOffset;
    }

    uint32_t Model::GetTangentOffset() const
    {
        return m_tangentOffset;
    }

    uint32_t Model::GetBlendWeightOffset() const
    {
        return m_blendWeightOffset;
    }

    Gfx::IndexType Model::GetIndexType() const
    {
        return Gfx::IndexType::UNSIGNED_INT;
    }
 
    void BlendWeight::AddJointData( uint32_t id, float w )
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

    glm::mat4 JointTransform::GetLocalTransformMatrix() const
    {
        glm::mat4 T = glm::translate( glm::mat4( 1 ), position );
        glm::mat4 R = glm::toMat4( rotation );
        glm::mat4 S = glm::scale( glm::mat4( 1 ), scale );
        return T * R * S;
    }

    JointTransform JointTransform::Interpolate( const JointTransform& end, float t )
    {
        JointTransform ret;
        ret.position = ( 1.0f - t ) * position + t * end.position;
        ret.rotation = glm::normalize( glm::slerp( rotation, end.rotation, t ) );
        ret.scale    = ( 1.0f - t ) * scale + t * end.scale;

        return ret;
    }
        
    void Skeleton::Serialize( std::ofstream& outFile ) const
    {
        serialize::Write( outFile, joints.size() );
        for ( const auto& joint : joints )
        {
            serialize::Write( outFile, joint.name );
            serialize::Write( outFile, joint.inverseBindTransform );
            serialize::Write( outFile, joint.children );
        }
    }

    void Skeleton::Deserialize( char*& buffer )
    {
        size_t numJoints;
        serialize::Read( buffer, numJoints );
        joints.resize( numJoints );
        for ( size_t i = 0; i < numJoints; ++i )
        {
            serialize::Read( buffer, joints[i].name );
            serialize::Read( buffer, joints[i].inverseBindTransform );
            serialize::Read( buffer, joints[i].children );
        }
    }

} // namespace Progression