#include "resource/skinned_model.hpp"
#include "openFBX/ofbx.h"
#include "core/assert.hpp"
#include "core/time.hpp"
#include "utils/logger.hpp"
#include "graphics/vulkan.hpp"

static glm::mat4 AssimpToGlmMat4( const aiMatrix4x4& AssimpMatrix )
{
    glm::mat4 m;

    m[0][0] = AssimpMatrix.a1; m[0][1] = AssimpMatrix.a2; m[0][2] = AssimpMatrix.a3; m[0][3] = AssimpMatrix.a4;
    m[1][0] = AssimpMatrix.b1; m[1][1] = AssimpMatrix.b2; m[1][2] = AssimpMatrix.b3; m[1][3] = AssimpMatrix.b4;
    m[2][0] = AssimpMatrix.c1; m[2][1] = AssimpMatrix.c2; m[2][2] = AssimpMatrix.c3; m[2][3] = AssimpMatrix.c4;
    m[3][0] = AssimpMatrix.d1; m[3][1] = AssimpMatrix.d2; m[3][2] = AssimpMatrix.d3; m[3][3] = AssimpMatrix.d4;

    return glm::transpose( m );
}

static glm::mat4 Assimp3x3ToGlmMat4( const aiMatrix3x3& AssimpMatrix )
{
    glm::mat4 m;

    m[0][0] = AssimpMatrix.a1; m[0][1] = AssimpMatrix.a2; m[0][2] = AssimpMatrix.a3; m[0][3] = 0.0f;
    m[1][0] = AssimpMatrix.b1; m[1][1] = AssimpMatrix.b2; m[1][2] = AssimpMatrix.b3; m[1][3] = 0.0f;
    m[2][0] = AssimpMatrix.c1; m[2][1] = AssimpMatrix.c2; m[2][2] = AssimpMatrix.c3; m[2][3] = 0.0f;
    m[3][0] = 0.0f           ; m[3][1] = 0.0f           ; m[3][2] = 0.0f           ; m[3][3] = 1.0f;

    return glm::transpose( m );
}

static glm::vec3 AssimpToGlmVec3( const aiVector3D& v )
{
    return { v.x, v.y, v.z };
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
        std::vector< float > vertexData( 3 * vertices.size() + 3 * normals.size() + 2 * uvs.size() + 8 * vertexBoneData.size() );
        char* dst = (char*) vertexData.data();
        memcpy( dst, vertices.data(), vertices.size() * sizeof( glm::vec3 ) );
        dst += vertices.size() * sizeof( glm::vec3 );
        memcpy( dst, normals.data(), normals.size() * sizeof( glm::vec3 ) );
        dst += normals.size() * sizeof( glm::vec3 );
        memcpy( dst, uvs.data(), uvs.size() * sizeof( glm::vec2 ) );
        dst += uvs.size() * sizeof( glm::vec2 );
        memcpy( dst, vertexBoneData.data(), vertexBoneData.size() * 2 * sizeof( glm::vec4 ) );
        LOG( "Num floats = ", vertexData.size() );
        vertexBuffer = Gfx::g_renderState.device.NewBuffer( vertexData.size() * sizeof( float ), vertexData.data(), BUFFER_TYPE_VERTEX, MEMORY_TYPE_DEVICE_LOCAL );
        indexBuffer  = Gfx::g_renderState.device.NewBuffer( indices.size() * sizeof ( uint32_t ), indices.data(), BUFFER_TYPE_INDEX, MEMORY_TYPE_DEVICE_LOCAL );

        m_numVertices           = static_cast< uint32_t >( vertices.size() );
        m_normalOffset          = m_numVertices * sizeof( glm::vec3 );
        m_uvOffset              = m_normalOffset + m_numVertices * sizeof( glm::vec3 );
        m_vertexBoneDataOffset  = static_cast< uint32_t >( m_uvOffset + uvs.size() * sizeof( glm::vec2 ) );

        LOG( "m_numVertices = ",    m_numVertices );
        LOG( "m_normalOffset = ",   m_normalOffset );
        LOG( "m_uvOffset = ",       m_uvOffset );
        LOG( "m_vertexBoneDataOffset = ", m_vertexBoneDataOffset );
        LOG( "normals.size() = ",   normals.size() );
        LOG( "uvs.size() = ",       uvs.size() );

        // Free();
    }

    void SkinnedModel::Free( bool cpuCopy, bool gpuCopy )
    {
        if ( cpuCopy )
        {
            m_numVertices = static_cast< uint32_t >( vertices.size() );
            vertices.shrink_to_fit();
            normals.shrink_to_fit();
            uvs.shrink_to_fit();
            indices.shrink_to_fit();
            vertexBoneData.shrink_to_fit();
            vertexBoneData.shrink_to_fit();
        }

        if ( gpuCopy )
        {
            vertexBuffer.Free();
            indexBuffer.Free();
            m_numVertices = 0;
            m_normalOffset = m_uvOffset = m_vertexBoneDataOffset = ~0u;
        }
    }

    uint32_t FindPosition( float AnimationTime, const aiNodeAnim* pNodeAnim )
    {    
        for ( uint32_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++ )
        {
            if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime)
            {
                return i;
            }
        }
    
        assert( 0 );

        return 0;
    }


    uint32_t FindRotation( float AnimationTime, const aiNodeAnim* pNodeAnim )
    {
        assert( pNodeAnim->mNumRotationKeys > 0 );

        for ( uint32_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++ )
        {
            if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime)
            {
                return i;
            }
        }
    
        assert( 0 );

        return 0;
    }


    uint32_t FindScaling( float AnimationTime, const aiNodeAnim* pNodeAnim )
    {
        assert(pNodeAnim->mNumScalingKeys > 0);
    
        for ( uint32_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++ )
        {
            if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime )
            {
                return i;
            }
        }
    
        assert( 0 );

        return 0;
    }

    void CalcInterpolatedPosition( aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim )
    {
        if ( pNodeAnim->mNumPositionKeys == 1 )
        {
            Out = pNodeAnim->mPositionKeys[0].mValue;
            return;
        }
            
        uint32_t PositionIndex = FindPosition(AnimationTime, pNodeAnim);
        uint32_t NextPositionIndex = (PositionIndex + 1);
        assert( NextPositionIndex < pNodeAnim->mNumPositionKeys );

        float DeltaTime = (float)( pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime );
        float Factor = ( AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime ) / DeltaTime;
        assert( Factor >= 0.0f && Factor <= 1.0f );

        const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
        const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
        aiVector3D Delta = End - Start;
        Out = Start + Factor * Delta;
    }


    void CalcInterpolatedRotation( aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim )
    {
	    // we need at least two values to interpolate...
        if ( pNodeAnim->mNumRotationKeys == 1 )
        {
            Out = pNodeAnim->mRotationKeys[0].mValue;
            return;
        }
    
        uint32_t RotationIndex = FindRotation( AnimationTime, pNodeAnim );
        uint32_t NextRotationIndex = ( RotationIndex + 1 );
        assert( NextRotationIndex < pNodeAnim->mNumRotationKeys );

        float DeltaTime = (float)( pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime );
        float Factor = ( AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime ) / DeltaTime;
        assert( Factor >= 0.0f && Factor <= 1.0f );

        const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
        const aiQuaternion& EndRotationQ   = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;    
        aiQuaternion::Interpolate( Out, StartRotationQ, EndRotationQ, Factor );
        Out = Out.Normalize();
    }


    void CalcInterpolatedScaling( aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim )
    {
        if ( pNodeAnim->mNumScalingKeys == 1 )
        {
            Out = pNodeAnim->mScalingKeys[0].mValue;
            return;
        }

        uint32_t ScalingIndex = FindScaling(AnimationTime, pNodeAnim );
        uint32_t NextScalingIndex = (ScalingIndex + 1);
        assert( NextScalingIndex < pNodeAnim->mNumScalingKeys );

        float DeltaTime = (float)( pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime );
        float Factor = ( AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime ) / DeltaTime;
        assert( Factor >= 0.0f && Factor <= 1.0f );

        const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
        const aiVector3D& End   = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
        aiVector3D Delta = End - Start;
        Out = Start + Factor * Delta;
    }

    const aiNodeAnim* FindNodeAnim( const aiAnimation* pAnimation, const std::string NodeName )
    {
        for ( uint32_t i = 0; i < pAnimation->mNumChannels; i++ )
        {
            const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];
        
            if ( std::string( pNodeAnim->mNodeName.data ) == NodeName )
            {
                return pNodeAnim;
            }
        }
    
        return NULL;
    }

    void SkinnedModel::ReadNodeHeirarchy( float AnimationTime, const aiNode* pNode, const glm::mat4& ParentTransform )
    {    
        std::string NodeName( pNode->mName.data );
    
        const aiAnimation* pAnimation = m_scene->mAnimations[0];
        
        glm::mat4 NodeTransformation( AssimpToGlmMat4( pNode->mTransformation ) );
     
        const aiNodeAnim* pNodeAnim = FindNodeAnim( pAnimation, NodeName );
    
        if ( pNodeAnim )
        {
            // Interpolate scaling and generate scaling transformation matrix
            aiVector3D Scaling;
            CalcInterpolatedScaling( Scaling, AnimationTime, pNodeAnim );
            glm::mat4 ScalingM( 1 );
            glm::scale( ScalingM, glm::vec3( Scaling.x, Scaling.y, Scaling.z ) );
        
            // Interpolate rotation and generate rotation transformation matrix
            aiQuaternion RotationQ;
            CalcInterpolatedRotation( RotationQ, AnimationTime, pNodeAnim );        
            glm::mat4 RotationM = Assimp3x3ToGlmMat4( RotationQ.GetMatrix() );

            // Interpolate translation and generate translation transformation matrix
            aiVector3D Translation;
            CalcInterpolatedPosition(Translation, AnimationTime, pNodeAnim);
            glm::mat4 TranslationM( 1 );
            glm::translate( TranslationM, glm::vec3( Translation.x, Translation.y, Translation.z ) );
        
            // Combine the above transformations
            NodeTransformation = TranslationM * RotationM * ScalingM;
        }
       
        glm::mat4 GlobalTransformation = ParentTransform * NodeTransformation;
    
        if ( m_boneMapping.find( NodeName ) != m_boneMapping.end() )
        {
            uint32_t boneIndex = m_boneMapping[NodeName];
            bones[boneIndex].finalTransformation = globalInverseTransform * GlobalTransformation * bones[boneIndex].offset;
        }
    
        for ( uint32_t i = 0; i < pNode->mNumChildren; i++ )
        {
            ReadNodeHeirarchy( AnimationTime, pNode->mChildren[i], GlobalTransformation );
        }
    }

    void SkinnedModel::TransformBones( std::vector< glm::mat4 >& finalTransforms, glm::mat4 modelMatrix )
    {
        finalTransforms.resize( bones.size(), glm::mat4( 1 ) );

        glm::mat4 startTransform( 1 );
        float ticksPerSecond = m_scene->mAnimations[0]->mTicksPerSecond != 0 ? (float) m_scene->mAnimations[0]->mTicksPerSecond : 25.0f;
        float timeInTicks = Time::Time() / 1000.0f * ticksPerSecond;
        float animationTime = fmod( timeInTicks, (float) m_scene->mAnimations[0]->mDuration );
        
        ReadNodeHeirarchy( animationTime, m_scene->mRootNode, startTransform );
        for ( size_t i = 0 ; i < bones.size() ; i++ )
        {
            finalTransforms[i] = modelMatrix * bones[i].finalTransformation;
        }
    }

    bool SkinnedModel::LoadFBX( const std::string& filename, std::shared_ptr< SkinnedModel >& model )
    {
        Assimp::Importer importer;
        const aiScene* scene = model->m_importer.ReadFile( filename.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices );
        model->m_scene = scene;
        if ( !scene )
        {
            LOG_ERR( "Error parsing FBX file '", filename.c_str(), "': ", model->m_importer.GetErrorString() );
            return false;
        }

        model->globalInverseTransform = glm::inverse( AssimpToGlmMat4( scene->mRootNode->mTransformation ) );

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
        model->vertexBoneData.resize( numVertices );
        LOG( "Num vertices = ", numVertices );
        LOG( "Num indices = ", numIndices );
        
        // Initialize the meshes in the scene one by one
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

            LOG( "Mesh[", meshIdx, "] Num bones: ", paiMesh->mNumBones );
            for ( uint32_t boneIdx = 0; boneIdx < paiMesh->mNumBones ; ++boneIdx )
            {
                uint32_t vertexBoneIndex = 0;
                std::string boneName( paiMesh->mBones[boneIdx]->mName.data );

                if ( model->m_boneMapping.find( boneName ) == model->m_boneMapping.end() )
                {
                    // Allocate an index for a new bone
                    vertexBoneIndex = static_cast< uint32_t >( model->bones.size() );
                    model->bones.push_back( {} );
                    model->bones[vertexBoneIndex].offset = AssimpToGlmMat4( paiMesh->mBones[boneIdx]->mOffsetMatrix );
                    model->bones[vertexBoneIndex].name   = boneName;
                    model->m_boneMapping[boneName] = vertexBoneIndex;
                }
                else
                {
                    vertexBoneIndex = model->m_boneMapping[boneName];
                }
        
                for ( unsigned int weightIdx = 0; weightIdx < paiMesh->mBones[boneIdx]->mNumWeights; ++weightIdx )
                {
                    uint32_t vertexID = model->meshes[meshIdx].m_startVertex + paiMesh->mBones[boneIdx]->mWeights[weightIdx].mVertexId;
                    float weight  = paiMesh->mBones[boneIdx]->mWeights[weightIdx].mWeight;
                    model->vertexBoneData[vertexID].AddBoneData( vertexBoneIndex, weight );
                }
            }
    
            // Populate the index buffer
            for ( size_t iIdx = 0 ; iIdx < paiMesh->mNumFaces ; ++iIdx )
            {
                const aiFace& face = paiMesh->mFaces[iIdx];
                PG_ASSERT( face.mNumIndices == 3 );
                model->indices.push_back( face.mIndices[0] );
                model->indices.push_back( face.mIndices[1] );
                model->indices.push_back( face.mIndices[2] );
            }
        }

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

        LOG( "Num bones: ", model->bones.size() );
        LOG( "Num vertexBoneData: ", model->vertexBoneData.size() );
        for ( size_t meshIdx = 0; meshIdx < model->meshes.size(); ++meshIdx )
        {
            LOG( "Mesh[", meshIdx, "].m_startIndex = ", model->meshes[meshIdx].m_startIndex );
            LOG( "Mesh[", meshIdx, "].m_startVertex = ", model->meshes[meshIdx].m_startVertex );
            LOG( "Mesh[", meshIdx, "].m_numIndices = ", model->meshes[meshIdx].m_numIndices );
        }

        for ( size_t i = 0; i < model->vertexBoneData.size(); ++i )
        {
            auto& data = model->vertexBoneData[i];
            float sum = data.weights[0] + data.weights[1] + data.weights[2] + data.weights[3];
            data.weights /= sum;
            // LOG( "Bone Vertex[", i, "]: joints = ", data.joints.x, " ", data.joints.y, " ", data.joints.z, " ", data.joints.w, ", weights = ", data.weights );
        }
        model->RecalculateAABB();
        model->UploadToGpu();

        return true;
    }


    //bool SkinnedModel::LoadFBX( const std::string& filename, std::vector< std::shared_ptr< SkinnedModel > >& models )
    //{
    //    ofbx::IScene* fbxScene;
    //    {
    //        FILE* fp = fopen( filename.c_str(), "rb" );
	   //     if ( !fp )
    //        {
    //            LOG_ERR( "Could not open file '", filename, "'" );
    //            return false;
    //        }

	   //     fseek( fp, 0, SEEK_END );
	   //     long file_size = ftell( fp );
	   //     fseek( fp, 0, SEEK_SET );
	   //     auto* content = new ofbx::u8[file_size];
	   //     fread( content, 1, file_size, fp );
	   //     fbxScene = ofbx::load( (ofbx::u8*)content, file_size, (ofbx::u64) ofbx::LoadFlags::TRIANGULATE );
	   //     if( !fbxScene )
    //        {
    //            LOG_ERR( "OFBX ERROR: ", ofbx::getError() );
    //            return false;
    //        }
    //        delete[] content;
    //        fclose( fp );
    //    }

    //    LOG( "fbxScene->getMeshCount = ",  fbxScene->getMeshCount() );

    //    int obj_idx = 0;
	   // int indices_offset = 0;
	   // int normals_offset = 0;
	   // int mesh_count = fbxScene->getMeshCount();
    //    models.resize( mesh_count );
    //    for ( int meshIdx = 0; meshIdx < mesh_count; ++meshIdx )
	   // {
    //        std::shared_ptr< SkinnedModel > skinnedModel = std::make_shared< SkinnedModel >();
    //        models[meshIdx] = skinnedModel;
    //        skinnedModel->name = filename + "_mesh_" + std::to_string( meshIdx );

		  //  const ofbx::Mesh& mesh = *fbxScene->getMesh( meshIdx );
		  //  const ofbx::Geometry& geom = *mesh.getGeometry();
		  //  int vertex_count = geom.getVertexCount();
		  //  const ofbx::Vec3* vertices = geom.getVertices();

    //        LOG( "Mesh[", meshIdx, "] numMaterials = ", mesh.getMaterialCount() );
    //        LOG( "geom[", meshIdx, "] vertex count = ", geom.getVertexCount() );
    //        LOG( "geom[", meshIdx, "] getIndexCount = ", geom.getIndexCount() );

    //        const ofbx::Skin* skin = geom.getSkin();
		  //  if ( skin )
		  //  {
    //            std::vector< const ofbx::Object* > bones;
    //            bones.reserve( 256 );
    //            LOG( "Skin cluster count = ", skin->getClusterCount() );
			 //   for ( int clusterIdx = 0; clusterIdx < skin->getClusterCount(); ++clusterIdx )
			 //   {
				//    const ofbx::Cluster* cluster = skin->getCluster( clusterIdx );
    //                // LOG( "Cluster[", clusterIdx, "]: ", cluster->getWeightsCount(), " = ", cluster->getIndicesCount() );
				//    InsertHierarchy( bones, cluster->getLink() );
			 //   }
    //            LOG( "Num bones before considering animations: ", bones.size() );
    //            for (int i = 0, n = fbxScene->getAnimationStackCount(); i < n; ++i )
	   //         {
		  //          const ofbx::AnimationStack* stack = fbxScene->getAnimationStack( i );
		  //          for ( int j = 0; stack->getLayer(j); ++j )
		  //          {
			 //           const ofbx::AnimationLayer* layer = stack->getLayer( j );
			 //           for (int k = 0; layer->getCurveNode(k); ++k)
			 //           {
				//            const ofbx::AnimationCurveNode* node = layer->getCurveNode( k );
				//            if ( node->getBone() )
    //                        {
    //                            InsertHierarchy( bones, node->getBone());
    //                        }
			 //           }
		  //          }
	   //         }
    //            LOG( "Num bones after considering animations: ", bones.size() );

    //            std::sort( bones.begin(), bones.end() );
    //            bones.erase( std::unique( bones.begin(), bones.end() ), bones.end() );
    //            SortBones( bones );
    //            LOG( "num bones: ", bones.size() );

    //            skinnedModel->skeleton.resize( bones.size() );
    //            skinnedModel->vertexBoneData = std::vector< VertexBoneData >( vertex_count );
    //            std::vector< uint8_t > vertexBoneCounts( vertex_count, 0 );
    //            for (int i = 0, c = skin->getClusterCount(); i < c; ++i)
	   //         {
		  //          const ofbx::Cluster* cluster = skin->getCluster( i );
		  //          if ( cluster->getIndicesCount() == 0 )
    //                {
    //                    continue;
    //                }
		  //          auto it = std::find( bones.begin(), bones.end(), cluster->getLink() );
		  //          
    //                int joint = static_cast< int >( it - bones.begin() );
    //                PG_ASSERT( joint < bones.size() );
    //                skinnedModel->skeleton[joint].offset              = OfbxToGlmMat4( bones[joint]->getLocalTransform() );
    //                skinnedModel->skeleton[joint].finalTransformation = OfbxToGlmMat4( bones[joint]->getGlobalTransform() );
    //                it = std::find( bones.begin(), bones.end(), bones[joint]->getParent() );
    //                skinnedModel->skeleton[joint].parentIndex = static_cast< uint32_t >( it - bones.begin() );
    //                PG_ASSERT( skinnedModel->skeleton[joint].parentIndex < bones.size() );

		  //          const int* cp_indices = cluster->getIndices();
		  //          const double* weights = cluster->getWeights();
		  //          for (int j = 0; j < cluster->getIndicesCount(); ++j )
		  //          {
			 //           int idx             = cp_indices[j];
			 //           float weight        = (float)weights[j];
			 //           VertexBoneData& s   = skinnedModel->vertexBoneData[idx];
    //                    auto& count = vertexBoneCounts[idx];
			 //           if ( count < 4 )
			 //           {
				//            s.weights[count] = weight;
				//            s.joints[count] = joint;
				//            ++count;
			 //           }
			 //           else
			 //           {
				//            int min = 0;
				//            for ( int m = 1; m < 4; ++m )
				//            {
				//	            if ( s.weights[m] < s.weights[min] )
    //                            {
    //                                min = m;
    //                            }
				//            }

				//            if ( s.weights[min] < weight )
				//            {
				//	            s.weights[min] = weight;
				//	            s.joints[min] = joint;
				//            }
			 //           }
		  //          }
	   //         }

	   //         for ( VertexBoneData& s : skinnedModel->vertexBoneData )
	   //         {
		  //          float sum = s.weights.x + s.weights.y + s.weights.z + s.weights.w;
    //                PG_ASSERT( sum > 0 );
		  //          s.weights /= sum;
	   //         }

    //            for ( size_t vertex = 0; vertex < skinnedModel->vertexBoneData.size(); ++vertex )
    //            {
    //                const auto& vertexBoneData = skinnedModel->vertexBoneData[vertex];
    //                //LOG( "vertexBoneData[", vertex, "]: weights = ", vertexBoneData.weights, ", joints[", (int)vertexBoneCounts[vertex], "] = ", vertexBoneData.joints.x, " ", vertexBoneData.joints.y, " ", vertexBoneData.joints.z, " ", vertexBoneData.joints.w );
    //            }

    //            for ( size_t boneIdx = 0; boneIdx < skinnedModel->skeleton.size(); ++boneIdx )
    //            {
    //                for ( size_t boneIdx2 = 0; boneIdx2 < skinnedModel->skeleton.size(); ++boneIdx2 )
    //                {
    //                    if ( boneIdx == boneIdx2 )
    //                    {
    //                        continue;
    //                    }
    //                    if ( skinnedModel->skeleton[boneIdx2].parentIndex == boneIdx )
    //                    {
    //                        skinnedModel->skeleton[boneIdx].children.push_back( (uint32_t) boneIdx2 );
    //                    }
    //                }
    //            }

    //            skinnedModel->rootBone.offset               = OfbxToGlmMat4( geom.getLocalTransform() );
    //            skinnedModel->rootBone.finalTransformation  = OfbxToGlmMat4( geom.getGlobalTransform() );
    //            skinnedModel->rootBone.parentIndex          = ~0u;
    //            LOG( "Geometry local transform:\n", OfbxToGlmMat4( geom.getLocalTransform() ) );
    //            LOG( "Geometry global transform:\n", OfbxToGlmMat4( geom.getGlobalTransform() ) );
    //            for ( size_t boneIdx = 0; boneIdx < skinnedModel->skeleton.size(); ++boneIdx )
    //            {
    //                if ( skinnedModel->skeleton[boneIdx].parentIndex == ~0u )
    //                {
    //                    skinnedModel->rootBone.children.push_back( (uint32_t) boneIdx );
    //                }
    //            }
    //            std::string children;
    //            for ( size_t childIdx = 0; childIdx < skinnedModel->rootBone.children.size(); ++childIdx )
    //            {
    //                children += " " + std::to_string( skinnedModel->rootBone.children[childIdx] );
    //            }
    //            LOG( "ROOT Bone parent: ", skinnedModel->rootBone.parentIndex );
    //            LOG( "ROOT Bone children: ", children, "\n" );
    //            //auto it = std::find( bones.begin(), bones.end(), bones[0 );
    //            //int joint = static_cast< int >( it - bones.begin() );
    //            for ( size_t boneIdx = 0; boneIdx < skinnedModel->skeleton.size() - 1; ++boneIdx )
    //            {
    //                const auto& bone = skinnedModel->skeleton[boneIdx];
    //                LOG( "Bone[", boneIdx, "] local transform:\n", bone.offset );
    //                LOG( "Bone[", boneIdx, "] global transform:\n", bone.finalTransformation );
    //                /*if ( bone.finalTransformation == glm::mat4( 0 ) )
    //                {*/
    //                    glm::vec3 p = OfbxToGlmVec3( bones[boneIdx]->getLocalTranslation() );
    //                    glm::vec3 r = OfbxToGlmVec3( bones[boneIdx]->getLocalRotation() );
    //                    glm::vec3 s = OfbxToGlmVec3( bones[boneIdx]->getLocalScaling() );

    //                    LOG( "Bone[", boneIdx, "]: position: ", p, ", rotation = ", r, ", scaling = ", s );
    //                // }
    //                std::string children;
    //                for ( size_t childIdx = 0; childIdx < skinnedModel->skeleton[boneIdx].children.size(); ++childIdx )
    //                {
    //                    children += " " + std::to_string( skinnedModel->skeleton[boneIdx].children[childIdx] );
    //                }
    //                LOG( "Bone[", boneIdx, "] parent: ", skinnedModel->skeleton[boneIdx].parentIndex );
    //                LOG( "Bone[", boneIdx, "] children: ", children, "\n" );
    //            }
		  //  }

    //        skinnedModel->meshes.resize( mesh.getMaterialCount() );
    //        for ( int mtlIdx = 0; mtlIdx < mesh.getMaterialCount(); ++mtlIdx )
    //        {
    //            const ofbx::Material* fbxMat = mesh.getMaterial( mtlIdx );
    //            auto pgMat = std::make_shared< Material >();
    //            pgMat = std::make_shared< Material >();
    //            pgMat->Ka = glm::vec3( 0 );
    //            pgMat->Kd = glm::vec3( fbxMat->getDiffuseColor().r, fbxMat->getDiffuseColor().g, fbxMat->getDiffuseColor().b );
    //            pgMat->Ks = glm::vec3( fbxMat->getSpecularColor().r, fbxMat->getSpecularColor().g, fbxMat->getSpecularColor().b );
    //            pgMat->Ke = glm::vec3( 0 );
    //            pgMat->Ns = 50;
    //            char tmp[256];
    //            if ( fbxMat->getTexture( ofbx::Texture::TextureType::DIFFUSE ) != nullptr )
    //            {
    //                fbxMat->getTexture( ofbx::Texture::TextureType::DIFFUSE )->getRelativeFileName().toString( tmp );
    //                LOG( "Material DIFFUSE texture name: '", tmp, "'" );
    //            }
    //            if ( fbxMat->getTexture( ofbx::Texture::TextureType::NORMAL ) != nullptr )
    //            {
    //                fbxMat->getTexture( ofbx::Texture::TextureType::NORMAL )->getRelativeFileName().toString( tmp );
    //                LOG( "Material NORMAL texture name: '", tmp, "'" );
    //            }
    //            if ( fbxMat->getTexture( ofbx::Texture::TextureType::SPECULAR ) != nullptr )
    //            {
    //                fbxMat->getTexture( ofbx::Texture::TextureType::SPECULAR )->getRelativeFileName().toString( tmp );
    //                LOG( "Material SPECULAR texture name: '", tmp, "'" );
    //            }
    //            skinnedModel->meshes[mtlIdx].material = pgMat;
    //        }

    //        skinnedModel->vertices.resize( vertex_count );
    //        for ( int i = 0; i < vertex_count; ++i )
		  //  {
			 //   ofbx::Vec3 v = vertices[i];
    //            skinnedModel->vertices[i] = { v.x, v.y, v.z };
		  //  }

		  //  const ofbx::Vec3* normals = geom.getNormals();
		  //  const ofbx::Vec2* uvs     = geom.getUVs();
    //        if ( normals )
    //        {
    //            PG_ASSERT( vertex_count == geom.getIndexCount() );
    //        }

		  //  const int* faceIndices    = geom.getFaceIndices();
    //        const int* materials      = geom.getMaterials();
		  //  int index_count           = geom.getIndexCount();
    //        // skinnedModel->vertices.clear();
    //    
		  //  for ( int i = 0; i < index_count; i += 3 ) // - 3 because the last vertex is -infinity on this dragon model
		  //  {
    //            PG_ASSERT( faceIndices[i + 2] < 0 ); // triangulated

			 //   int idx1 = indices_offset + faceIndices[i + 0];
			 //   int idx2 = indices_offset + faceIndices[i + 1];
			 //   int idx3 = indices_offset + -faceIndices[i + 2] - 1;

    //            /*int vertex_idx1 = indices_offset + idx1;
			 //   int vertex_idx2 = indices_offset + idx2;
			 //   int vertex_idx3 = indices_offset + idx3;

    //            skinnedModel->vertices.emplace_back( vertices[vertex_idx1].x, vertices[vertex_idx1].y, vertices[vertex_idx1].z );
    //            skinnedModel->vertices.emplace_back( vertices[vertex_idx2].x, vertices[vertex_idx2].y, vertices[vertex_idx2].z );
    //            skinnedModel->vertices.emplace_back( vertices[vertex_idx3].x, vertices[vertex_idx3].y, vertices[vertex_idx3].z );*/

    //            if ( uvs )
			 //   {
				//    int uv_idx1 = normals_offset + ( i + 0 );
				//    int uv_idx2 = normals_offset + ( i + 1 );
				//    int uv_idx3 = normals_offset + ( i + 2 );
    //                skinnedModel->uvs.emplace_back( uvs[uv_idx1].x, uvs[uv_idx1].y );
    //                skinnedModel->uvs.emplace_back( uvs[uv_idx2].x, uvs[uv_idx2].y );
    //                skinnedModel->uvs.emplace_back( uvs[uv_idx3].x, uvs[uv_idx3].y );
			 //   }

			 //   if ( normals )
			 //   {
				//    int normal_idx1 = normals_offset + ( i + 0 );
				//    int normal_idx2 = normals_offset + ( i + 1 );
				//    int normal_idx3 = normals_offset + ( i + 2 );
    //                skinnedModel->normals.emplace_back( normals[normal_idx1].x, normals[normal_idx1].y, normals[normal_idx1].z );
    //                skinnedModel->normals.emplace_back( normals[normal_idx2].x, normals[normal_idx2].y, normals[normal_idx2].z );
    //                skinnedModel->normals.emplace_back( normals[normal_idx3].x, normals[normal_idx3].y, normals[normal_idx3].z );
			 //   }

    //            SkinnedMesh& skinnedMesh = skinnedModel->meshes[materials[i / 3]];
    //            skinnedMesh.indices.push_back( idx1 );
    //            skinnedMesh.indices.push_back( idx2 );
    //            skinnedMesh.indices.push_back( idx3 );
		  //  }

    //        indices_offset += vertex_count;
		  //  normals_offset += index_count;
		  //  ++obj_idx;

    //        for ( size_t meshIdx = 0; meshIdx < skinnedModel->meshes.size(); ++meshIdx )
    //        {
    //            if ( skinnedModel->meshes[meshIdx].indices.empty() )
    //            {
    //                skinnedModel->meshes.erase( skinnedModel->meshes.begin() + meshIdx );
    //                --meshIdx;
    //            }
    //        }
    //        if ( !normals )
    //        {
    //            skinnedModel->RecalculateNormals();
    //        }

    //        LOG( "SkinnedModel.numVertices = ", skinnedModel->vertices.size() );
    //        LOG( "SkinnedModel.numNormals  = ", skinnedModel->normals.size() );
    //        LOG( "SkinnedModel.numUVs      = ", skinnedModel->uvs.size() );
    //        for ( size_t meshIdx = 0; meshIdx < skinnedModel->meshes.size(); ++meshIdx )
    //        {
    //            LOG( "Mesh[", meshIdx, "].numIndices  = ", skinnedModel->meshes[meshIdx].indices.size() );
    //        }

    //        // pgModel->RecalculateBB( true );
    //        skinnedModel->UploadToGpu();
	   // }

    //    return true;
    //}

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

    uint32_t SkinnedModel::GetVertexBoneDataOffset() const
    {
        return m_vertexBoneDataOffset;
    }

    Gfx::IndexType SkinnedModel::GetIndexType() const
    {
        return Gfx::IndexType::UNSIGNED_INT;
    }

} // namespace Progression