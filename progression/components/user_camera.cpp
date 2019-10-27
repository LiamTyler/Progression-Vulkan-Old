#include "components/user_camera.hpp"
#include "core/input.hpp"
#include "core/time.hpp"

namespace Progression
{

UserCameraComponent::UserCameraComponent( Camera* camera, float ms, float ts, float ma ) :
  Component( camera ), moveSpeed( ms ), turnSpeed( ts ), maxAngle( glm::radians( ma ) ),
  velocity( glm::vec3( 0 ) )
{
}

void UserCameraComponent::Start()
{
}

void UserCameraComponent::Update()
{
    float dt       = Time::DeltaTime();
    Camera& camera = *(Camera*) gameObject;

    if ( Input::GetKeyDown( PG_K_W ) )
        velocity.z = 1;
    if ( Input::GetKeyDown( PG_K_S ) )
        velocity.z = -1;
    if ( Input::GetKeyDown( PG_K_D ) )
        velocity.x = 1;

    if ( Input::GetKeyDown( PG_K_A ) )
        velocity.x = -1;
    if ( Input::GetKeyDown( PG_K_LEFT_CONTROL ) )
        velocity.y = -1;
    if ( Input::GetKeyDown( PG_K_SPACE ) )
        velocity.y = 1;
    if ( Input::GetKeyDown( PG_K_LEFT_SHIFT ) )
        moveSpeed *= 10;

    if ( Input::GetKeyUp( PG_K_W ) || Input::GetKeyUp( PG_K_S ) )
        velocity.z = 0;
    if ( Input::GetKeyUp( PG_K_D ) || Input::GetKeyUp( PG_K_A ) )
        velocity.x = 0;
    if ( Input::GetKeyUp( PG_K_LEFT_CONTROL ) || Input::GetKeyUp( PG_K_SPACE ) )
        velocity.y = 0;
    if ( Input::GetKeyUp( PG_K_LEFT_SHIFT ) )
        moveSpeed *= .1;

    glm::ivec2 dMouse = -Input::GetMouseChange();
    glm::vec3 dRotation( glm::vec3( dMouse.y, dMouse.x, 0 ) );

    glm::vec3 rot = camera.transform.rotation + dRotation * turnSpeed;
    rot.x         = std::fmax( -maxAngle, std::fmin( maxAngle, rot.x ) );
    glm::vec3 pos = camera.transform.position +
                    dt * moveSpeed *
                      ( velocity.x * camera.GetRightDir() + velocity.y * camera.GetUpDir() +
                        velocity.z * camera.GetForwardDir() );

    camera.UpdateFrustum( pos, rot );
}

void UserCameraComponent::Stop()
{
}

} // namespace Progression