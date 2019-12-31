#include "core/animation_system.hpp"
#include "core/assert.hpp"
#include "core/scene.hpp"
#include "core/time.hpp"
#include "components/animation_component.hpp"
#include "components/skinned_renderer.hpp"
#include "graphics/debug_marker.hpp"
#include "graphics/render_system.hpp"
#include "graphics/vulkan.hpp"
#include "resource/resource_manager.hpp"
#include "resource/shader.hpp"
#include "resource/model.hpp"
#include "utils/logger.hpp"
#include <list>

using namespace Progression::Gfx;

extern struct Progression::RenderSystem::GBufferPassData gBufferPassData;

static std::list< std::pair< uint32_t, uint32_t > > s_freeList;

namespace Progression
{

namespace AnimationSystem
{

RenderData renderData;

bool Init()
{
    s_freeList = { { 0, MAX_ANIMATOR_NUM_TRANSFORMS } };
    renderData.gpuBoneBuffer = g_renderState.device.NewBuffer( sizeof( glm::mat4 ) * MAX_ANIMATOR_NUM_TRANSFORMS,
        BUFFER_TYPE_STORAGE, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT, "Bone Transforms" );
    renderData.gpuBoneBuffer.Map();

    VkDescriptorPoolSize poolSize[1] = {};
    poolSize[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize[0].descriptorCount = 1;

    renderData.descriptorPool = g_renderState.device.NewDescriptorPool( 1, poolSize, 1, "animation system" );

    auto vertShader = ResourceManager::Get< Shader >( "animatedModelsVert" );
    auto fragShader = ResourceManager::Get< Shader >( "gBufferFrag" );

    std::vector< DescriptorSetLayoutData > descriptorSetData = vertShader->reflectInfo.descriptorSetLayouts;
    descriptorSetData.insert( descriptorSetData.end(), fragShader->reflectInfo.descriptorSetLayouts.begin(), fragShader->reflectInfo.descriptorSetLayouts.end() );
    auto combined = CombineDescriptorSetLayouts( descriptorSetData );

    renderData.descriptorSetLayouts        = g_renderState.device.NewDescriptorSetLayouts( combined );
    renderData.animationBonesDescriptorSet = renderData.descriptorPool.NewDescriptorSet( renderData.descriptorSetLayouts[2], "skeletal animation" );

    std::vector< VkWriteDescriptorSet > writeDescriptorSets;
	std::vector< VkDescriptorBufferInfo > bufferDescriptors;
    bufferDescriptors =
    {
        DescriptorBufferInfo( renderData.gpuBoneBuffer ),
    };
    writeDescriptorSets =
    {
        WriteDescriptorSet( renderData.animationBonesDescriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &bufferDescriptors[0] ),
    };
    g_renderState.device.UpdateDescriptorSets( static_cast< uint32_t >( writeDescriptorSets.size() ), writeDescriptorSets.data() );

    VertexBindingDescriptor bindingDescs[] =
    {
        VertexBindingDescriptor( 0, sizeof( glm::vec3 ) ),
        VertexBindingDescriptor( 1, sizeof( glm::vec3 ) ),
        VertexBindingDescriptor( 2, sizeof( glm::vec2 ) ),
        VertexBindingDescriptor( 3, sizeof( glm::vec3 ) ),
        VertexBindingDescriptor( 4, 2 * sizeof( glm::vec4 ) ),
    };

    VertexAttributeDescriptor attribDescs[] =
    {
        VertexAttributeDescriptor( 0, 0, BufferDataType::FLOAT3, 0 ),
        VertexAttributeDescriptor( 1, 1, BufferDataType::FLOAT3, 0 ),
        VertexAttributeDescriptor( 2, 2, BufferDataType::FLOAT2, 0 ),
        VertexAttributeDescriptor( 3, 3, BufferDataType::FLOAT3, 0 ),
        VertexAttributeDescriptor( 4, 4, BufferDataType::FLOAT4, 0 ),
        VertexAttributeDescriptor( 5, 4, BufferDataType::UINT4, sizeof( glm::vec4 ) ),
    };

    PipelineDescriptor pipelineDesc;
    pipelineDesc.renderPass             = &gBufferPassData.renderPass;
    pipelineDesc.descriptorSetLayouts   = renderData.descriptorSetLayouts;
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 5, bindingDescs, 6, attribDescs );
    pipelineDesc.viewport               = FullScreenViewport();
    pipelineDesc.viewport.height        = -pipelineDesc.viewport.height;
    pipelineDesc.viewport.y             = -pipelineDesc.viewport.height;
    pipelineDesc.scissor                = FullScreenScissor();
    pipelineDesc.shaders[0]             = vertShader.get();
    pipelineDesc.shaders[1]             = fragShader.get();

    renderData.animatedPipeline = g_renderState.device.NewPipeline( pipelineDesc, "animation" );
    if ( !renderData.animatedPipeline )
    {
        LOG_ERR( "Could not create animated model pipeline" );
        return false;
    }

    return true;
}

void Shutdown()
{
    g_renderState.device.WaitForIdle();
    renderData.gpuBoneBuffer.UnMap();
    renderData.gpuBoneBuffer.Free();
    FreeDescriptorSetLayouts( renderData.descriptorSetLayouts );
    renderData.descriptorPool.Free();
    renderData.animatedPipeline.Free();
}

void Update( Scene* scene )
{
    scene->registry.view< Animator >().each([&]( const entt::entity e, Animator& comp )
    {
        if ( comp.animation && comp.animationTime < comp.animation->duration )
        {
            PG_ASSERT( comp.GetModel()->skeleton.joints.size() > 0, "Trying to animate a skeleton with 0 bones" );
            comp.animationTime = comp.animationTime + Time::DeltaTime();            
            //comp.animationTime = comp.animationTime + 1.0f/165;            
            if ( comp.loop && comp.animationTime >= comp.animation->duration / comp.animation->ticksPerSecond )
            {
                comp.animationTime   = std::fmod( comp.animationTime, comp.animation->duration / comp.animation->ticksPerSecond );
                comp.currentKeyFrame = 0;
            }

            for ( ; comp.currentKeyFrame < comp.animation->keyFrames.size(); ++comp.currentKeyFrame )
            {
                if ( comp.animationTime < comp.animation->keyFrames[comp.currentKeyFrame].time / comp.animation->ticksPerSecond )
                {
                    break;
                }
            }
            uint32_t nextFrameIndex = comp.currentKeyFrame % (uint32_t) comp.animation->keyFrames.size();
            uint32_t prevFrameIndex = comp.currentKeyFrame - 1;
            if ( comp.currentKeyFrame == 0 )
            {
               prevFrameIndex = static_cast< uint32_t >( comp.animation->keyFrames.size() );
            }

            auto& prevKeyFrame     = comp.animation->keyFrames[prevFrameIndex];
            auto& nextKeyFrame     = comp.animation->keyFrames[nextFrameIndex];
            float keyFrameDuration = ( nextKeyFrame.time - prevKeyFrame.time ) / comp.animation->ticksPerSecond;
            float progress         = ( comp.animationTime - prevKeyFrame.time / comp.animation->ticksPerSecond ) / keyFrameDuration;
            for ( uint32_t i = 0; i < comp.GetModel()->skeleton.joints.size(); ++i )
            {
                const JointTransform interpolatedTransform = prevKeyFrame.jointSpaceTransforms[i].Interpolate( nextKeyFrame.jointSpaceTransforms[i], progress );
                comp.transformBuffer[i] = interpolatedTransform.GetLocalTransformMatrix();
            }

            comp.GetModel()->ApplyPoseToJoints( 0, glm::mat4( 1 ), comp.transformBuffer );
        }
    });
}

void UploadToGpu( Scene* scene )
{
    scene->registry.view< Animator >().each([&]( const entt::entity e, Animator& comp )
    {
        if ( comp.animation && comp.animationTime < comp.animation->duration )
        {
            auto& boneTransforms = comp.transformBuffer;
            glm::mat4* ptr = (glm::mat4*) renderData.gpuBoneBuffer.MappedPtr();
            memcpy( ptr + comp.GetTransformSlot(), boneTransforms.data(), boneTransforms.size() * sizeof( glm::mat4 ) );
        }
    });
}

uint32_t AllocateGPUTransforms( uint32_t numTransforms )
{
    for ( auto it = s_freeList.begin(); it != s_freeList.end(); ++it )
    {
        uint32_t slot = it->first;
        if ( numTransforms <= it->second )
        {
            if ( numTransforms == it->second )
            {
                s_freeList.erase( it );
            }
            else
            {
                it->first  = it->first + numTransforms;
                it->second = it->second - numTransforms;
            }
            return slot | numTransforms << 16;
        }
    }

    PG_ASSERT( false, "Not enough contiguous memory available for animation transforms!" );
    return ~0u;
}

void FreeGPUTransforms( uint32_t id )
{
    uint32_t slot = GET_ANIMATOR_SLOT_FROM_ID( id );
    uint32_t size = GET_ANIMATOR_SIZE_FROM_ID( id );
    auto it = s_freeList.begin();
    while ( it != s_freeList.end() && it->first < slot )
    {
        if ( slot == it->first + it->second )
        {
            it->second += size;
            auto next = std::next( it );
            if ( next != s_freeList.end() && slot + size == next->first )
            {
                it->second += next->second;
                s_freeList.erase( next );
            }
            return;
        }
        ++it;
    }

    if ( it != s_freeList.end() && slot + size == it->first )
    {
        it->first   = slot;
        it->second += size;
    }
    else
    {
        s_freeList.insert( it, { slot, size } );
    }
}

void OnAnimatorConstruction( entt::entity entity, entt::registry& registry, Animator& animator )
{
    if ( !animator.GetModel() )
    {
        return;
    }

    animator.AssignNewModel( animator.GetModel() );
}

void OnAnimatorDestruction( entt::entity entity, entt::registry& registry )
{
    Animator& a = registry.get< Animator >( entity );
    a.ReleaseModel();
}

void PrintFreeList()
{
    LOG( "Animation Free List:" );
    for ( const auto& [ slot, size ] : s_freeList )
    {
        LOG( "Slot = ", slot, ", size = ", size );
    }
    LOG( "" );
}

} // namespace AnimationSystem
} // namespace Progression