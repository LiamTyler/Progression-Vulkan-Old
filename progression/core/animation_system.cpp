#include "core/animation_system.hpp"
#include "core/assert.hpp"
#include "core/scene.hpp"
#include "core/time.hpp"
#include "components/animation_component.hpp"
#include "components/skinned_renderer.hpp"
#include "graphics/vulkan.hpp"
#include "resource/resource_manager.hpp"
#include "resource/shader.hpp"
#include "resource/skinned_model.hpp"
#include "utils/logger.hpp"
#include <list>

using namespace Progression::Gfx;

static std::list< std::pair< uint32_t, uint32_t > > s_freeList;

namespace Progression
{

namespace AnimationSystem
{

RenderData renderData;

bool Init()
{
    s_freeList = { { 0, MAX_ANIMATOR_NUM_TRANSFORMS } };
    uint32_t numFrames = Gfx::MAX_FRAMES_IN_FLIGHT + 1;
    renderData.gpuBoneBuffers.resize( numFrames );
    for ( auto& buf : renderData.gpuBoneBuffers )
    {
        buf = g_renderState.device.NewBuffer( sizeof( glm::mat4 ) * MAX_ANIMATOR_NUM_TRANSFORMS,
            BUFFER_TYPE_STORAGE, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT );
    }

    VkDescriptorPoolSize poolSize[1] = {};
    poolSize[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize[0].descriptorCount = 1 * numFrames;

    renderData.descriptorPool = g_renderState.device.NewDescriptorPool( 1, poolSize, numFrames );

    auto animatedModelsVert    = ResourceManager::Get< Shader >( "animatedModelsVert" );
    auto forwardBlinnPhongFrag = ResourceManager::Get< Shader >( "forwardBlinnPhongFrag" );

    std::vector< DescriptorSetLayoutData > descriptorSetData = animatedModelsVert->reflectInfo.descriptorSetLayouts;
    descriptorSetData.insert( descriptorSetData.end(), forwardBlinnPhongFrag->reflectInfo.descriptorSetLayouts.begin(), forwardBlinnPhongFrag->reflectInfo.descriptorSetLayouts.end() );
    auto combined = CombineDescriptorSetLayouts( descriptorSetData );

    renderData.descriptorSetLayouts         = g_renderState.device.NewDescriptorSetLayouts( combined );
    renderData.animationBonesDescriptorSets = renderData.descriptorPool.NewDescriptorSets( numFrames, renderData.descriptorSetLayouts[2] );

    for ( uint32_t i = 0; i < numFrames; i++ )
    {
        VkWriteDescriptorSet descriptorWrite = {};
        VkDescriptorBufferInfo boneDataBufferInfo = {};
        boneDataBufferInfo.buffer = renderData.gpuBoneBuffers[i].GetHandle();
        boneDataBufferInfo.offset = 0;
        boneDataBufferInfo.range  = VK_WHOLE_SIZE;
        descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet           = renderData.animationBonesDescriptorSets[i].GetHandle();
        descriptorWrite.dstBinding       = 0;
        descriptorWrite.dstArrayElement  = 0;
        descriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrite.descriptorCount  = 1;
        descriptorWrite.pBufferInfo      = &boneDataBufferInfo;
    
        g_renderState.device.UpdateDescriptorSets( 1, &descriptorWrite );
    }

    VertexBindingDescriptor bindingDesc[5];
    bindingDesc[0].binding   = 0;
    bindingDesc[0].stride    = sizeof( glm::vec3 );
    bindingDesc[0].inputRate = VertexInputRate::PER_VERTEX;
    bindingDesc[1].binding   = 1;
    bindingDesc[1].stride    = sizeof( glm::vec3 );
    bindingDesc[1].inputRate = VertexInputRate::PER_VERTEX;
    bindingDesc[2].binding   = 2;
    bindingDesc[2].stride    = sizeof( glm::vec2 );
    bindingDesc[2].inputRate = VertexInputRate::PER_VERTEX;
    bindingDesc[3].binding   = 3;
    bindingDesc[3].stride    = 2 * sizeof( glm::vec4 );
    bindingDesc[3].inputRate = VertexInputRate::PER_VERTEX;
    
    std::array< VertexAttributeDescriptor, 5 > attribDescs;
    attribDescs[0].binding  = 0;
    attribDescs[0].location = 0;
    attribDescs[0].format   = BufferDataType::FLOAT3;
    attribDescs[0].offset   = 0;
    attribDescs[1].binding  = 1;
    attribDescs[1].location = 1;
    attribDescs[1].format   = BufferDataType::FLOAT3;
    attribDescs[1].offset   = 0;
    attribDescs[2].binding  = 2;
    attribDescs[2].location = 2;
    attribDescs[2].format   = BufferDataType::FLOAT2;
    attribDescs[2].offset   = 0;
    attribDescs[3].binding  = 3;
    attribDescs[3].location = 3;
    attribDescs[3].format   = BufferDataType::FLOAT4;
    attribDescs[3].offset   = 0;
    attribDescs[4].binding  = 3;
    attribDescs[4].location = 4;
    attribDescs[4].format   = BufferDataType::UINT4;
    attribDescs[4].offset   = sizeof( glm::vec4 );

    PipelineDescriptor pipelineDesc;
    pipelineDesc.renderPass             = &g_renderState.renderPass;
    pipelineDesc.descriptorSetLayouts   = renderData.descriptorSetLayouts;
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 4, bindingDesc, 5, attribDescs.data() );
    pipelineDesc.viewport               = FullScreenViewport();
    pipelineDesc.scissor                = FullScreenScissor();
    pipelineDesc.shaders[0]             = animatedModelsVert.get();
    pipelineDesc.shaders[1]             = forwardBlinnPhongFrag.get();
    pipelineDesc.numShaders             = 2;

    renderData.animatedPipeline = g_renderState.device.NewPipeline( pipelineDesc );
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
    for ( auto& buf : renderData.gpuBoneBuffers )
    {
        buf.Free();
    }
    for ( auto& layout : renderData.descriptorSetLayouts )
    {
        layout.Free();
    }
    renderData.descriptorPool.Free();
    renderData.animatedPipeline.Free();
}

void Update( Scene* scene )
{
    scene->registry.view< Animator >().each([&]( const entt::entity e, Animator& comp )
    {
        if ( comp.animation && comp.animationTime < comp.animation->duration )
        {
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
            for ( uint32_t i = 0; i < comp.GetModel()->joints.size(); ++i )
            {
                const JointTransform interpolatedTransform = prevKeyFrame.jointSpaceTransforms[i].Interpolate( nextKeyFrame.jointSpaceTransforms[i], progress );
                comp.transformBuffer[i] = interpolatedTransform.GetLocalTransformMatrix();
            }

            comp.GetModel()->ApplyPoseToJoints( 0, glm::mat4( 1 ), comp.transformBuffer );
        }
    });
}

void UploadToGpu( Scene* scene, uint32_t frameIndex )
{
    scene->registry.view< Animator >().each([&]( const entt::entity e, Animator& comp )
    {
        if ( comp.animation && comp.animationTime < comp.animation->duration )
        {
            auto& boneTransforms = comp.transformBuffer;
            renderData.gpuBoneBuffers[frameIndex].Map();
            glm::mat4* ptr = (glm::mat4*) renderData.gpuBoneBuffers[frameIndex].MappedPtr();
            memcpy( ptr + comp.GetTransformSlot(), boneTransforms.data(), boneTransforms.size() * sizeof( glm::mat4 ) );
            renderData.gpuBoneBuffers[frameIndex].UnMap();
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