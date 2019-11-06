#include "core/animation_system.hpp"
#include "core/scene.hpp"
#include "core/time.hpp"
#include "components/animation_component.hpp"
#include "graphics/vulkan.hpp"
#include "resource/resource_manager.hpp"
#include "resource/shader.hpp"
#include "resource/skinned_model.hpp"
#include "utils/logger.hpp"

using namespace Progression::Gfx;

static std::vector< Buffer > s_gpuBoneBuffers;
static Pipeline s_animatedPipeline;
static std::vector< DescriptorSetLayout > s_descriptorSetLayouts;
std::vector< DescriptorSet > animationBonesDescriptorSets;
static DescriptorPool s_descriptorPool;

namespace Progression
{

namespace AnimationSystem
{

bool Init()
{
    uint32_t numFrames = Gfx::MAX_FRAMES_IN_FLIGHT + 1;
    s_gpuBoneBuffers.resize( numFrames );
    for ( auto& buf : s_gpuBoneBuffers )
    {
        buf = g_renderState.device.NewBuffer( sizeof( glm::mat4 ) * 1000,
            BUFFER_TYPE_STORAGE, MEMORY_TYPE_HOST_VISIBLE | MEMORY_TYPE_HOST_COHERENT );
    }

    VkDescriptorPoolSize poolSize[3] = {};
    poolSize[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize[1].descriptorCount = 1 * numFrames;

    s_descriptorPool = g_renderState.device.NewDescriptorPool( 1, poolSize, numFrames );

    auto animatedModelsVert    = ResourceManager::Get< Shader >( "animatedModelsVert" );
    auto forwardBlinnPhongFrag = ResourceManager::Get< Shader >( "forwardBlinnPhongFrag" );

    std::vector< DescriptorSetLayoutData > descriptorSetData = animatedModelsVert->reflectInfo.descriptorSetLayouts;
    descriptorSetData.insert( descriptorSetData.end(), forwardBlinnPhongFrag->reflectInfo.descriptorSetLayouts.begin(), forwardBlinnPhongFrag->reflectInfo.descriptorSetLayouts.end() );
    auto combined = CombineDescriptorSetLayouts( descriptorSetData );

    s_descriptorSetLayouts       = g_renderState.device.NewDescriptorSetLayouts( combined );
    animationBonesDescriptorSets = s_descriptorPool.NewDescriptorSets( numFrames, s_descriptorSetLayouts[2] );

    for ( uint32_t i = 0; i < numFrames; i++ )
    {
        VkWriteDescriptorSet descriptorWrite = {};
        VkDescriptorBufferInfo boneDataBufferInfo = {};
        boneDataBufferInfo.buffer = s_gpuBoneBuffers[i].GetHandle();
        boneDataBufferInfo.offset = 0;
        boneDataBufferInfo.range  = VK_WHOLE_SIZE;
        descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet           = animationBonesDescriptorSets[i].GetHandle();
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
    bindingDesc[3].stride    = 1 * sizeof( glm::vec4 ); // 2?
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
    pipelineDesc.descriptorSetLayouts   = s_descriptorSetLayouts;
    pipelineDesc.vertexDescriptor       = VertexInputDescriptor::Create( 4, bindingDesc, 5, attribDescs.data() );
    pipelineDesc.viewport               = FullScreenViewport();
    pipelineDesc.scissor                = FullScreenScissor();
    pipelineDesc.shaders[0]             = animatedModelsVert.get();
    pipelineDesc.shaders[1]             = forwardBlinnPhongFrag.get();
    pipelineDesc.numShaders             = 2;

    s_animatedPipeline = g_renderState.device.NewPipeline( pipelineDesc );
    if ( !s_animatedPipeline )
    {
        LOG_ERR( "Could not create animated model pipeline" );
        return false;
    }

    return true;
}

void Shutdown()
{
    g_renderState.device.WaitForIdle();
    for ( auto& buf : s_gpuBoneBuffers )
    {
        buf.Free();
    }
    for ( auto& layout : s_descriptorSetLayouts )
    {
        layout.Free();
    }
    s_animatedPipeline.Free();
}

void Update( Scene* scene, uint32_t frameIndex )
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
               prevFrameIndex = comp.animation->keyFrames.size();
            }

            auto& prevKeyFrame     = comp.animation->keyFrames[prevFrameIndex];
            auto& nextKeyFrame     = comp.animation->keyFrames[nextFrameIndex];
            float keyFrameDuration = ( nextKeyFrame.time - prevKeyFrame.time ) / comp.animation->ticksPerSecond;
            float progress         = ( comp.animationTime - prevKeyFrame.time / comp.animation->ticksPerSecond ) / keyFrameDuration;
            for ( uint32_t i = 0; i < comp.model->joints.size(); ++i )
            {
                const JointTransform interpolatedTransform = prevKeyFrame.jointSpaceTransforms[i].Interpolate( nextKeyFrame.jointSpaceTransforms[i], progress );
                comp.model->joints[i].modelSpaceTransform = interpolatedTransform.GetLocalTransformMatrix();
            }

            comp.model->ApplyPoseToJoints( 0, glm::mat4( 1 ) );

            std::vector< glm::mat4 > boneTransforms;
            comp.model->GetCurrentPose( boneTransforms );
            void* data = s_gpuBoneBuffers[frameIndex].Map();
            memcpy( (char*)data, boneTransforms.data(), boneTransforms.size() * sizeof( glm::mat4 ) );
            s_gpuBoneBuffers[frameIndex].UnMap();
        }
    });

    
}

} // namespace AnimationSystem
} // namespace Progression