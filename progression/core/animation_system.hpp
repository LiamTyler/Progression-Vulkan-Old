#pragma once
#include "graphics/graphics_api.hpp"

namespace Progression
{

class Scene;

namespace AnimationSystem
{

struct RenderData
{
    std::vector< Gfx::Buffer > gpuBoneBuffers;
    Gfx::Pipeline animatedPipeline;
    std::vector< Gfx::DescriptorSetLayout > descriptorSetLayouts;
    std::vector< Gfx::DescriptorSet > animationBonesDescriptorSets;
    Gfx::DescriptorPool descriptorPool;
};

extern RenderData renderData;

bool Init();

void Shutdown();

void Update( Scene* scene );

void UploadToGpu( Scene* scene, uint32_t frameIndex );

} // namespace AnimationSystem
} // namespace Progression