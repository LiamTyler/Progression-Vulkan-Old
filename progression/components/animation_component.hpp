#pragma once

#include <memory>

namespace Progression
{
    class SkinnedModel;
    struct Animation;

    struct Animator
    {
        SkinnedModel* model      = nullptr;
        Animation* animation     = nullptr;
        float animationTime      = 0;
        bool loop                = true;
        uint32_t currentKeyFrame = 0;
    };

} // namespace Progression