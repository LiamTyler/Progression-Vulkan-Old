#pragma once

#include "core/frustum.hpp"

namespace Progression
{

class Camera
{
public:
    Camera();

    void UpdateFrustum();
    void UpdateOrientationVectors();
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();

    glm::mat4 GetV() const;
    glm::mat4 GetP() const;
    glm::mat4 GetVP() const;
    glm::vec3 GetForwardDir() const;
    glm::vec3 GetUpDir() const;
    glm::vec3 GetRightDir() const;
    Frustum   GetFrustum() const;

    glm::vec3 position = glm::vec3( 0 );
    glm::vec3 rotation = glm::vec3( 0 );
    float fov          = glm::radians( 45.0f );
    float aspectRatio  = 16.0f / 9.0f;
    float nearPlane    = 0.1f;
    float farPlane     = 100.0f;

protected:
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;
    glm::vec3 m_currDir;
    glm::vec3 m_currUp;
    glm::vec3 m_currRight;
    Frustum m_frustum;
};

} // namespace Progression
