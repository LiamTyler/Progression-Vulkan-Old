#include "components/animation_component.hpp"
#include "core/animation_system.hpp"
#include "core/assert.hpp"
#include "resource/model.hpp"

namespace Progression
{

Animator::Animator( Model* m ) :
    model( m )
{
    if ( m )
    {
        transformBuffer.resize( m->skeleton.joints.size() );
    }
}

void Animator::AssignNewModel( Model* m )
{
    PG_ASSERT( m );
    if ( animationSysSlotID != ~0u )
    {
        AnimationSystem::FreeGPUTransforms( animationSysSlotID );
    }
    model = m;
    animationSysSlotID = AnimationSystem::AllocateGPUTransforms( static_cast< uint32_t >( m->skeleton.joints.size() ) );
    transformBuffer.resize( m->skeleton.joints.size() );
}

void Animator::ReleaseModel()
{
    if ( animationSysSlotID != ~0u )
    {
        AnimationSystem::FreeGPUTransforms( animationSysSlotID );
        animationSysSlotID = ~0u;
    }
    model = nullptr;
}

uint32_t Animator::GetTransformSlot() const
{
    return GET_ANIMATOR_SLOT_FROM_ID( animationSysSlotID );
}

Model* Animator::GetModel() const
{
    return model;
}

} // namespace Progression