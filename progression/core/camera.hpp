#pragma once

#include "core/frustum.hpp"

namespace Progression
{

class Camera
{
public:
    Camera( const glm::vec3& pos = glm::vec3( 0 ),
            const glm::vec3& rot = glm::vec3( 0 ),
            float fovy           = glm::radians( 45.0f ),
            float a              = 16.0f / 9.0f,
            float np             = .1f,
            float fp             = 100.0f );

    void UpdateFrustum( const glm::vec3& position, const glm::vec3& rotation );
    void UpdateOrientationVectors();
    void UpdateViewMatrix();

    float     GetFOV() const;
    float     GetAspectRatio() const;
    float     GetNearPlane() const;
    float     GetFarPlane() const;
    glm::mat4 GetV() const;
    glm::mat4 GetP() const;
    glm::mat4 GetVP() const;
    glm::vec3 GetForwardDir() const;
    glm::vec3 GetUpDir() const;
    glm::vec3 GetRightDir() const;
    Frustum   GetFrustum() const;

    void SetFOV( float f );
    void SetAspectRatio( float a );
    void SetNearPlane( float p );
    void SetFarPlane( float p );

    glm::vec3 position;
    glm::vec3 rotation;

protected:
    void UpdateProjectionMatrix();
    float m_fieldOfView;
    float m_aspectRatio;
    float m_nearPlane;
    float m_farPlane;
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;
    glm::vec3 m_currDir;
    glm::vec3 m_currUp;
    glm::vec3 m_currRight;
    Frustum m_frustum;
};

} // namespace Progression
