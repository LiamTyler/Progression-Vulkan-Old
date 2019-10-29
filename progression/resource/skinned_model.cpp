#include "resource/skinned_model.hpp"
#include "openFBX/ofbx.h"
#include "core/assert.hpp"
#include "utils/logger.hpp"
#include "graphics/vulkan.hpp"

static void InsertHierarchy( std::vector< const ofbx::Object* >& bones, const ofbx::Object* node )
{
	if ( !node || std::find( bones.begin(), bones.end(), node ) != bones.end() )
    {
        return;
    }
	ofbx::Object* parent = node->getParent();
	InsertHierarchy( bones, parent );
	bones.push_back( node );
}

static void SortBones( std::vector< const ofbx::Object* >& bones )
{
	int count = (int) bones.size();
	for ( int i = 0; i < count; ++i )
	{
		for ( int j = i + 1; j < count; ++j )
		{
			if ( bones[i]->getParent() == bones[j] )
			{
				const ofbx::Object* bone = bones[j];
                std::swap( bones[j], bones[bones.size() - 1] );
                bones.pop_back();
                bones.insert( bones.begin() + i, bone );
				--i;
				break;
			}
		}
	}

	/*for (const ofbx::Object*& bone : bones) {
		const int idx = meshes.find([&](const ImportMesh& mesh){
			return mesh.fbx == bone;
		});

		if (idx >= 0) {
			meshes[idx].is_skinned = true;
			meshes[idx].bone_idx = int(&bone - bones.begin());
		}
	}*/
}

static glm::mat4 OfbxToGlmMat4( const ofbx::Matrix& m )
{
    glm::mat4 ret;
    ret[0] = { m.m[0],  m.m[1],  m.m[2],  m.m[3] };
    ret[1] = { m.m[4],  m.m[5],  m.m[6],  m.m[7] };
    ret[2] = { m.m[8],  m.m[9],  m.m[10], m.m[11] };
    ret[3] = { m.m[12], m.m[13], m.m[14], m.m[15] };

    return ret;
}

static glm::vec3 OfbxToGlmVec3( const ofbx::Vec3& v )
{
    return { v.x, v.y, v.z };
}

namespace Progression
{

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

        for ( auto& mesh : meshes )
        {
            for ( size_t i = 0; i < mesh.indices.size(); i += 3 )
            {
                glm::vec3 v1 = vertices[mesh.indices[i + 0]];
                glm::vec3 v2 = vertices[mesh.indices[i + 1]];
                glm::vec3 v3 = vertices[mesh.indices[i + 2]];
                glm::vec3 n = glm::cross( v2 - v1, v3 - v1 );
                normals[mesh.indices[i + 0]] += n;
                normals[mesh.indices[i + 1]] += n;
                normals[mesh.indices[i + 2]] += n;
            }
        }

        for ( auto& normal : normals )
        {
            normal = glm::normalize( normal );
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
        // std::vector< glm::vec4 > weights( vertexBoneData.size() );
        // std::vector< glm::uvec4 > joints( vertexBoneData.size() );
        // for ( size_t i = 0; i < weights.size(); ++i )
        // {
        //     weights[i] = vertexBoneData[i].weights;
        //     joints[i] = vertexBoneData[i].joints;
        //     // LOG( "weight[", i, "] = ", weights[i].x + weights[i].y + weights[i].z + weights[i].w );
        // }
        // memcpy( dst, weights.data(), weights.size() * sizeof( glm::vec4 ) );
        // dst += weights.size() * sizeof( glm::vec4 );
        // memcpy( dst, joints.data(), joints.size() * sizeof( glm::uvec4 ) );
        memcpy( dst, vertexBoneData.data(), vertexBoneData.size() * 2 * sizeof( glm::vec4 ) );
        LOG( "Num floats = ", vertexData.size() );
        vertexBuffer = Gfx::g_renderState.device.NewBuffer( vertexData.size() * sizeof( float ), vertexData.data(), BUFFER_TYPE_VERTEX, MEMORY_TYPE_DEVICE_LOCAL );

        size_t totalIndices = 0;
        for ( const auto& mesh : meshes )
        {
            totalIndices += mesh.indices.size();
        }
        std::vector< uint32_t > indices( totalIndices );
        size_t currIndex = 0;
        for ( auto& mesh : meshes )
        {
            memcpy( (char*) indices.data() + currIndex * sizeof( uint32_t ), mesh.indices.data(), mesh.indices.size() * sizeof( uint32_t ) );
            mesh.m_numIndices = static_cast< uint32_t >( mesh.indices.size() );
            mesh.m_startIndex = static_cast< uint32_t >( currIndex );
            LOG( "mesh.m_numIndices = ", mesh.m_numIndices );
            LOG( "mesh.m_startIndex = ", mesh.m_startIndex );
            currIndex += mesh.indices.size();
        }
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
            for ( auto& mesh : meshes )
            {
                mesh.indices.shrink_to_fit();
            }
        }

        if ( gpuCopy )
        {
            vertexBuffer.Free();
            indexBuffer.Free();
            m_numVertices = 0;
            m_normalOffset = m_uvOffset = m_vertexBoneDataOffset = ~0u;
        }
    }

    void SkinnedModel::TransformChildren( uint32_t boneIdx, const glm::mat4& parentMatrix, std::vector< glm::mat4 >& finalTransforms )
    {
        auto transform = parentMatrix * skeleton[boneIdx].offset;
        finalTransforms[boneIdx] = transform;
        for ( const auto& child : skeleton[boneIdx].children )
        {
            TransformChildren( child, transform, finalTransforms );
        }
    }

    void SkinnedModel::TransformBones( std::vector< glm::mat4 >& finalTransforms, glm::mat4 modelMatrix )
    {
        finalTransforms.resize( skeleton.size() );
        for ( const auto& child : rootBone.children )
        {
            TransformChildren( child, glm::mat4( 1 ), finalTransforms );
        }
        for ( auto& transform : finalTransforms )
        {
            // transform = modelMatrix * glm::inverse( rootBone.finalTransformation ) * transform;
            transform = modelMatrix * transform;
        }
    }

    bool SkinnedModel::LoadFBX( const std::string& filename, std::vector< std::shared_ptr< SkinnedModel > >& models )
    {
        ofbx::IScene* fbxScene;
        {
            FILE* fp = fopen( filename.c_str(), "rb" );
	        if ( !fp )
            {
                LOG_ERR( "Could not open file '", filename, "'" );
                return false;
            }

	        fseek( fp, 0, SEEK_END );
	        long file_size = ftell( fp );
	        fseek( fp, 0, SEEK_SET );
	        auto* content = new ofbx::u8[file_size];
	        fread( content, 1, file_size, fp );
	        fbxScene = ofbx::load( (ofbx::u8*)content, file_size, (ofbx::u64) ofbx::LoadFlags::TRIANGULATE );
	        if( !fbxScene )
            {
                LOG_ERR( "OFBX ERROR: ", ofbx::getError() );
                return false;
            }
            delete[] content;
            fclose( fp );
        }

        LOG( "fbxScene->getMeshCount = ",  fbxScene->getMeshCount() );

        int obj_idx = 0;
	    int indices_offset = 0;
	    int normals_offset = 0;
	    int mesh_count = fbxScene->getMeshCount();
        models.resize( mesh_count );
        for ( int meshIdx = 0; meshIdx < mesh_count; ++meshIdx )
	    {
            std::shared_ptr< SkinnedModel > skinnedModel = std::make_shared< SkinnedModel >();
            models[meshIdx] = skinnedModel;
            skinnedModel->name = filename + "_mesh_" + std::to_string( meshIdx );

		    const ofbx::Mesh& mesh = *fbxScene->getMesh( meshIdx );
		    const ofbx::Geometry& geom = *mesh.getGeometry();
		    int vertex_count = geom.getVertexCount();
		    const ofbx::Vec3* vertices = geom.getVertices();

            LOG( "Mesh[", meshIdx, "] numMaterials = ", mesh.getMaterialCount() );
            LOG( "geom[", meshIdx, "] vertex count = ", geom.getVertexCount() );
            LOG( "geom[", meshIdx, "] getIndexCount = ", geom.getIndexCount() );

            const ofbx::Skin* skin = geom.getSkin();
		    if ( skin )
		    {
                std::vector< const ofbx::Object* > bones;
                bones.reserve( 256 );
                LOG( "Skin cluster count = ", skin->getClusterCount() );
			    for ( int clusterIdx = 0; clusterIdx < skin->getClusterCount(); ++clusterIdx )
			    {
				    const ofbx::Cluster* cluster = skin->getCluster( clusterIdx );
                    // LOG( "Cluster[", clusterIdx, "]: ", cluster->getWeightsCount(), " = ", cluster->getIndicesCount() );
				    InsertHierarchy( bones, cluster->getLink() );
			    }
                LOG( "Num bones before considering animations: ", bones.size() );
                for (int i = 0, n = fbxScene->getAnimationStackCount(); i < n; ++i )
	            {
		            const ofbx::AnimationStack* stack = fbxScene->getAnimationStack( i );
		            for ( int j = 0; stack->getLayer(j); ++j )
		            {
			            const ofbx::AnimationLayer* layer = stack->getLayer( j );
			            for (int k = 0; layer->getCurveNode(k); ++k)
			            {
				            const ofbx::AnimationCurveNode* node = layer->getCurveNode( k );
				            if ( node->getBone() )
                            {
                                InsertHierarchy( bones, node->getBone());
                            }
			            }
		            }
	            }
                LOG( "Num bones after considering animations: ", bones.size() );

                std::sort( bones.begin(), bones.end() );
                bones.erase( std::unique( bones.begin(), bones.end() ), bones.end() );
                SortBones( bones );
                LOG( "num bones: ", bones.size() );

                skinnedModel->skeleton.resize( bones.size() );
                skinnedModel->vertexBoneData = std::vector< VertexBoneData >( vertex_count );
                std::vector< uint8_t > vertexBoneCounts( vertex_count, 0 );
                for (int i = 0, c = skin->getClusterCount(); i < c; ++i)
	            {
		            const ofbx::Cluster* cluster = skin->getCluster( i );
		            if ( cluster->getIndicesCount() == 0 )
                    {
                        continue;
                    }
		            auto it = std::find( bones.begin(), bones.end(), cluster->getLink() );
		            
                    int joint = static_cast< int >( it - bones.begin() );
                    PG_ASSERT( joint < bones.size() );
                    skinnedModel->skeleton[joint].offset              = OfbxToGlmMat4( bones[joint]->getLocalTransform() );
                    skinnedModel->skeleton[joint].finalTransformation = OfbxToGlmMat4( bones[joint]->getGlobalTransform() );
                    it = std::find( bones.begin(), bones.end(), bones[joint]->getParent() );
                    skinnedModel->skeleton[joint].parentIndex = static_cast< uint32_t >( it - bones.begin() );
                    PG_ASSERT( skinnedModel->skeleton[joint].parentIndex < bones.size() );

		            const int* cp_indices = cluster->getIndices();
		            const double* weights = cluster->getWeights();
		            for (int j = 0; j < cluster->getIndicesCount(); ++j )
		            {
			            int idx             = cp_indices[j];
			            float weight        = (float)weights[j];
			            VertexBoneData& s   = skinnedModel->vertexBoneData[idx];
                        auto& count = vertexBoneCounts[idx];
			            if ( count < 4 )
			            {
				            s.weights[count] = weight;
				            s.joints[count] = joint;
				            ++count;
			            }
			            else
			            {
				            int min = 0;
				            for ( int m = 1; m < 4; ++m )
				            {
					            if ( s.weights[m] < s.weights[min] )
                                {
                                    min = m;
                                }
				            }

				            if ( s.weights[min] < weight )
				            {
					            s.weights[min] = weight;
					            s.joints[min] = joint;
				            }
			            }
		            }
	            }

	            for ( VertexBoneData& s : skinnedModel->vertexBoneData )
	            {
		            float sum = s.weights.x + s.weights.y + s.weights.z + s.weights.w;
                    PG_ASSERT( sum > 0 );
		            s.weights /= sum;
	            }

                for ( size_t vertex = 0; vertex < skinnedModel->vertexBoneData.size(); ++vertex )
                {
                    const auto& vertexBoneData = skinnedModel->vertexBoneData[vertex];
                    //LOG( "vertexBoneData[", vertex, "]: weights = ", vertexBoneData.weights, ", joints[", (int)vertexBoneCounts[vertex], "] = ", vertexBoneData.joints.x, " ", vertexBoneData.joints.y, " ", vertexBoneData.joints.z, " ", vertexBoneData.joints.w );
                }

                for ( size_t boneIdx = 0; boneIdx < skinnedModel->skeleton.size(); ++boneIdx )
                {
                    for ( size_t boneIdx2 = 0; boneIdx2 < skinnedModel->skeleton.size(); ++boneIdx2 )
                    {
                        if ( boneIdx == boneIdx2 )
                        {
                            continue;
                        }
                        if ( skinnedModel->skeleton[boneIdx2].parentIndex == boneIdx )
                        {
                            skinnedModel->skeleton[boneIdx].children.push_back( (uint32_t) boneIdx2 );
                        }
                    }
                }

                skinnedModel->rootBone.offset               = OfbxToGlmMat4( geom.getLocalTransform() );
                skinnedModel->rootBone.finalTransformation  = OfbxToGlmMat4( geom.getGlobalTransform() );
                skinnedModel->rootBone.parentIndex          = ~0u;
                LOG( "Geometry local transform:\n", OfbxToGlmMat4( geom.getLocalTransform() ) );
                LOG( "Geometry global transform:\n", OfbxToGlmMat4( geom.getGlobalTransform() ) );
                for ( size_t boneIdx = 0; boneIdx < skinnedModel->skeleton.size(); ++boneIdx )
                {
                    if ( skinnedModel->skeleton[boneIdx].parentIndex == ~0u )
                    {
                        skinnedModel->rootBone.children.push_back( (uint32_t) boneIdx );
                    }
                }
                std::string children;
                for ( size_t childIdx = 0; childIdx < skinnedModel->rootBone.children.size(); ++childIdx )
                {
                    children += " " + std::to_string( skinnedModel->rootBone.children[childIdx] );
                }
                LOG( "ROOT Bone parent: ", skinnedModel->rootBone.parentIndex );
                LOG( "ROOT Bone children: ", children, "\n" );
                //auto it = std::find( bones.begin(), bones.end(), bones[0 );
                //int joint = static_cast< int >( it - bones.begin() );
                for ( size_t boneIdx = 0; boneIdx < skinnedModel->skeleton.size() - 1; ++boneIdx )
                {
                    const auto& bone = skinnedModel->skeleton[boneIdx];
                    LOG( "Bone[", boneIdx, "] local transform:\n", bone.offset );
                    LOG( "Bone[", boneIdx, "] global transform:\n", bone.finalTransformation );
                    /*if ( bone.finalTransformation == glm::mat4( 0 ) )
                    {*/
                        glm::vec3 p = OfbxToGlmVec3( bones[boneIdx]->getLocalTranslation() );
                        glm::vec3 r = OfbxToGlmVec3( bones[boneIdx]->getLocalRotation() );
                        glm::vec3 s = OfbxToGlmVec3( bones[boneIdx]->getLocalScaling() );

                        LOG( "Bone[", boneIdx, "]: position: ", p, ", rotation = ", r, ", scaling = ", s );
                    // }
                    std::string children;
                    for ( size_t childIdx = 0; childIdx < skinnedModel->skeleton[boneIdx].children.size(); ++childIdx )
                    {
                        children += " " + std::to_string( skinnedModel->skeleton[boneIdx].children[childIdx] );
                    }
                    LOG( "Bone[", boneIdx, "] parent: ", skinnedModel->skeleton[boneIdx].parentIndex );
                    LOG( "Bone[", boneIdx, "] children: ", children, "\n" );
                }
		    }

            skinnedModel->meshes.resize( mesh.getMaterialCount() );
            for ( int mtlIdx = 0; mtlIdx < mesh.getMaterialCount(); ++mtlIdx )
            {
                const ofbx::Material* fbxMat = mesh.getMaterial( mtlIdx );
                auto pgMat = std::make_shared< Material >();
                pgMat = std::make_shared< Material >();
                pgMat->Ka = glm::vec3( 0 );
                pgMat->Kd = glm::vec3( fbxMat->getDiffuseColor().r, fbxMat->getDiffuseColor().g, fbxMat->getDiffuseColor().b );
                pgMat->Ks = glm::vec3( fbxMat->getSpecularColor().r, fbxMat->getSpecularColor().g, fbxMat->getSpecularColor().b );
                pgMat->Ke = glm::vec3( 0 );
                pgMat->Ns = 50;
                char tmp[256];
                if ( fbxMat->getTexture( ofbx::Texture::TextureType::DIFFUSE ) != nullptr )
                {
                    fbxMat->getTexture( ofbx::Texture::TextureType::DIFFUSE )->getRelativeFileName().toString( tmp );
                    LOG( "Material DIFFUSE texture name: '", tmp, "'" );
                }
                if ( fbxMat->getTexture( ofbx::Texture::TextureType::NORMAL ) != nullptr )
                {
                    fbxMat->getTexture( ofbx::Texture::TextureType::NORMAL )->getRelativeFileName().toString( tmp );
                    LOG( "Material NORMAL texture name: '", tmp, "'" );
                }
                if ( fbxMat->getTexture( ofbx::Texture::TextureType::SPECULAR ) != nullptr )
                {
                    fbxMat->getTexture( ofbx::Texture::TextureType::SPECULAR )->getRelativeFileName().toString( tmp );
                    LOG( "Material SPECULAR texture name: '", tmp, "'" );
                }
                skinnedModel->meshes[mtlIdx].material = pgMat;
            }

            skinnedModel->vertices.resize( vertex_count );
            for ( int i = 0; i < vertex_count; ++i )
		    {
			    ofbx::Vec3 v = vertices[i];
                skinnedModel->vertices[i] = { v.x, v.y, v.z };
		    }

		    const ofbx::Vec3* normals = geom.getNormals();
		    const ofbx::Vec2* uvs     = geom.getUVs();
            if ( normals )
            {
                PG_ASSERT( vertex_count == geom.getIndexCount() );
            }

		    const int* faceIndices    = geom.getFaceIndices();
            const int* materials      = geom.getMaterials();
		    int index_count           = geom.getIndexCount();
            // skinnedModel->vertices.clear();
        
		    for ( int i = 0; i < index_count; i += 3 ) // - 3 because the last vertex is -infinity on this dragon model
		    {
                PG_ASSERT( faceIndices[i + 2] < 0 ); // triangulated

			    int idx1 = indices_offset + faceIndices[i + 0];
			    int idx2 = indices_offset + faceIndices[i + 1];
			    int idx3 = indices_offset + -faceIndices[i + 2] - 1;

                /*int vertex_idx1 = indices_offset + idx1;
			    int vertex_idx2 = indices_offset + idx2;
			    int vertex_idx3 = indices_offset + idx3;

                skinnedModel->vertices.emplace_back( vertices[vertex_idx1].x, vertices[vertex_idx1].y, vertices[vertex_idx1].z );
                skinnedModel->vertices.emplace_back( vertices[vertex_idx2].x, vertices[vertex_idx2].y, vertices[vertex_idx2].z );
                skinnedModel->vertices.emplace_back( vertices[vertex_idx3].x, vertices[vertex_idx3].y, vertices[vertex_idx3].z );*/

                if ( uvs )
			    {
				    int uv_idx1 = normals_offset + ( i + 0 );
				    int uv_idx2 = normals_offset + ( i + 1 );
				    int uv_idx3 = normals_offset + ( i + 2 );
                    skinnedModel->uvs.emplace_back( uvs[uv_idx1].x, uvs[uv_idx1].y );
                    skinnedModel->uvs.emplace_back( uvs[uv_idx2].x, uvs[uv_idx2].y );
                    skinnedModel->uvs.emplace_back( uvs[uv_idx3].x, uvs[uv_idx3].y );
			    }

			    if ( normals )
			    {
				    int normal_idx1 = normals_offset + ( i + 0 );
				    int normal_idx2 = normals_offset + ( i + 1 );
				    int normal_idx3 = normals_offset + ( i + 2 );
                    skinnedModel->normals.emplace_back( normals[normal_idx1].x, normals[normal_idx1].y, normals[normal_idx1].z );
                    skinnedModel->normals.emplace_back( normals[normal_idx2].x, normals[normal_idx2].y, normals[normal_idx2].z );
                    skinnedModel->normals.emplace_back( normals[normal_idx3].x, normals[normal_idx3].y, normals[normal_idx3].z );
			    }

                SkinnedMesh& skinnedMesh = skinnedModel->meshes[materials[i / 3]];
                skinnedMesh.indices.push_back( idx1 );
                skinnedMesh.indices.push_back( idx2 );
                skinnedMesh.indices.push_back( idx3 );
		    }

            indices_offset += vertex_count;
		    normals_offset += index_count;
		    ++obj_idx;

            for ( size_t meshIdx = 0; meshIdx < skinnedModel->meshes.size(); ++meshIdx )
            {
                if ( skinnedModel->meshes[meshIdx].indices.empty() )
                {
                    skinnedModel->meshes.erase( skinnedModel->meshes.begin() + meshIdx );
                    --meshIdx;
                }
            }
            if ( !normals )
            {
                skinnedModel->RecalculateNormals();
            }

            LOG( "SkinnedModel.numVertices = ", skinnedModel->vertices.size() );
            LOG( "SkinnedModel.numNormals  = ", skinnedModel->normals.size() );
            LOG( "SkinnedModel.numUVs      = ", skinnedModel->uvs.size() );
            for ( size_t meshIdx = 0; meshIdx < skinnedModel->meshes.size(); ++meshIdx )
            {
                LOG( "Mesh[", meshIdx, "].numIndices  = ", skinnedModel->meshes[meshIdx].indices.size() );
            }

            // pgModel->RecalculateBB( true );
            skinnedModel->UploadToGpu();
	    }

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

    uint32_t SkinnedModel::GetVertexBoneDataOffset() const
    {
        return m_vertexBoneDataOffset;
    }

    Gfx::IndexType SkinnedModel::GetIndexType() const
    {
        return Gfx::IndexType::UNSIGNED_INT;
    }

} // namespace Progression