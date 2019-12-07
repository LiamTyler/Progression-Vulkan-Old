#pragma once
#include "graphics/graphics_api.hpp"
#include "components/animation_component.hpp"
#include "core/ecs.hpp"

#define MAX_ANIMATOR_NUM_TRANSFORMS 1000
#define GET_ANIMATOR_SLOT_FROM_ID( id ) ( id & 0xFFFF )
#define GET_ANIMATOR_SIZE_FROM_ID( id ) ( id >> 16 )

namespace Progression
{

class Scene;

namespace AnimationSystem
{

    struct RenderData
    {
        Gfx::Buffer gpuBoneBuffer;
        Gfx::Pipeline animatedPipeline;
        std::vector< Gfx::DescriptorSetLayout > descriptorSetLayouts;
        Gfx::DescriptorSet animationBonesDescriptorSet;
        Gfx::DescriptorPool descriptorPool;
    };

    extern RenderData renderData;

    bool Init();

    void Shutdown();

    void Update( Scene* scene );

    void UploadToGpu( Scene* scene );

    uint32_t AllocateGPUTransforms( uint32_t numTransforms );

    void FreeGPUTransforms( uint32_t id );

    void OnAnimatorConstruction( entt::entity entity, entt::registry& registry, Animator& animator );
    void OnAnimatorDestruction( entt::entity entity, entt::registry& registry );

} // namespace AnimationSystem
} // namespace Progression