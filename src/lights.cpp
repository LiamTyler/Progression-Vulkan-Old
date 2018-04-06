#include "include/lights.h"

Light::Light() : Light(
    glm::vec3(0.3, 0.3, 0.3),
    glm::vec3(0.7, 0.7, 0.7),
    glm::vec3(1.0, 1.0, 1.0))
{
}

Light::Light(glm::vec3 a, glm::vec3 d, glm::vec3 s) {
    Ia = a;
    Id = d;
    Is = s;
}

DirectionalLight::DirectionalLight() {
    direction = glm::vec3(0, 0, -1);
}

DirectionalLight::DirectionalLight(glm::vec3 dir) {
    direction = dir;
}

DirectionalLight::DirectionalLight(glm::vec3 dir,
        glm::vec3 a, glm::vec3 d, glm::vec3 s) : Light(a, d, s) {
    direction = dir;
}

PointLight::PointLight() {
    position = glm::vec3(0, 0, -1);
}

PointLight::PointLight(glm::vec3 pos) {
    position = pos;
}

PointLight::PointLight(glm::vec3 pos, glm::vec3 a, glm::vec3 d, glm::vec3 s) : Light(a, d, s) {
    position = pos;
}
