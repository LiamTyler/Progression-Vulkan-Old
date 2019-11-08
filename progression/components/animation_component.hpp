#pragma once

#include <vector>
#include "glm/mat4x4.hpp"

namespace Progression
{
    class SkinnedModel;
    struct Animation;

    struct Animator
    {
        Animator() = default;
        Animator( SkinnedModel* m );

        void AssignNewModel( SkinnedModel* m );
        void ReleaseModel();
        uint32_t GetTransformSlot() const;
        SkinnedModel* GetModel() const;

        Animation* animation        = nullptr;
        float animationTime         = 0;
        bool loop                   = true;
        uint32_t currentKeyFrame    = 0;
        std::vector< glm::mat4 > transformBuffer;

    private:
        uint32_t animationSysSlotID = ~0u;
        SkinnedModel* model         = nullptr;
    };

} // namespace Progression