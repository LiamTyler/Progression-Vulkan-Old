#include "core/camera.hpp"

namespace Progression
{

Camera::Camera(
  const glm::vec3& pos, const glm::vec3& rot, float fovy, float a, float np, float fp ) :
  position( pos ),
  rotation( rot ),
  m_fieldOfView( fovy ),
  m_aspectRatio( a ),
  m_nearPlane( np ),
  m_farPlane( fp )
{
    UpdateOrientationVectors();
    UpdateViewMatrix();
    UpdateProjectionMatrix();
}

void Camera::UpdateFrustum( const glm::vec3& pos, const glm::vec3& rot )
{
    position = pos;
    rotation = rot;
    UpdateOrientationVectors();
    UpdateViewMatrix();
    m_frustum.Update( m_fieldOfView, m_nearPlane, m_farPlane, m_aspectRatio, position, m_currDir, m_currUp,
                     m_currRight );
}

void Camera::UpdateOrientationVectors()
{
    glm::mat4 rot( 1 );
    // TODO: glm can do rotation around an axis already
    rot         = glm::rotate( rot, rotation.y, glm::vec3( 0, 1, 0 ) );
    rot         = glm::rotate( rot, rotation.x, glm::vec3( 1, 0, 0 ) );
    m_currDir   = glm::vec3( rot * glm::vec4( 0, 0, -1, 0 ) );
    m_currUp    = glm::vec3( rot * glm::vec4( 0, 1, 0, 0 ) );
    m_currRight = glm::cross( m_currDir, m_currUp );
}

void Camera::UpdateViewMatrix()
{
    m_viewMatrix = glm::lookAt( position, position + m_currDir, m_currUp );
}

void Camera::UpdateProjectionMatrix()
{
    m_projectionMatrix        = glm::perspective( m_fieldOfView, m_aspectRatio, m_nearPlane, m_farPlane );
    m_projectionMatrix[1][1] *= -1;
}

glm::mat4 Camera::GetV() const
{
    return m_viewMatrix;
}
glm::mat4 Camera::GetP() const
{
    return m_projectionMatrix;
}
glm::mat4 Camera::GetVP() const
{
    return m_projectionMatrix * m_viewMatrix;
}
float Camera::GetFOV() const
{
    return m_fieldOfView;
}
float Camera::GetAspectRatio() const
{
    return m_aspectRatio;
}
float Camera::GetNearPlane() const
{
    return m_nearPlane;
}
float Camera::GetFarPlane() const
{
    return m_farPlane;
}
glm::vec3 Camera::GetForwardDir() const
{
    return m_currDir;
}
glm::vec3 Camera::GetUpDir() const
{
    return m_currUp;
}
glm::vec3 Camera::GetRightDir() const
{
    return m_currRight;
}
Frustum Camera::GetFrustum() const
{
    return m_frustum;
}

void Camera::SetFOV( float f )
{
    m_fieldOfView = f;
    UpdateProjectionMatrix();
}

void Camera::SetAspectRatio( float a )
{
    m_aspectRatio = a;
    UpdateProjectionMatrix();
}

void Camera::SetNearPlane( float p )
{
    m_nearPlane = p;
    UpdateProjectionMatrix();
}

void Camera::SetFarPlane( float p )
{
    m_farPlane = p;
    UpdateProjectionMatrix();
}

} // namespace Progression
