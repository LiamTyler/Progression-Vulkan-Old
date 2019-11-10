#pragma once

#include <vector>
#include "glm/mat4x4.hpp"

namespace Progression
{
    class Model;
    struct Animation;

    struct Animator
    {
        Animator() = default;
        Animator( Model* m );

        void AssignNewModel( Model* m );
        void ReleaseModel();
        uint32_t GetTransformSlot() const;
        Model* GetModel() const;

        Animation* animation        = nullptr;
        float animationTime         = 0;
        bool loop                   = true;
        uint32_t currentKeyFrame    = 0;
        std::vector< glm::mat4 > transformBuffer;

    private:
        uint32_t animationSysSlotID = ~0u;
        Model* model         = nullptr;
    };

} // namespace Progression