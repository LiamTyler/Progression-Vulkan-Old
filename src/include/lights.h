#pragma once

#include "include/utils.h"

class Light {
    public:
        Light();
        Light(glm::vec3 a, glm::vec3 d, glm::vec3 s);

        glm::vec3 Ia;
        glm::vec3 Id;
        glm::vec3 Is;
};

class DirectionalLight : public Light {
    public:
        DirectionalLight();
        DirectionalLight(glm::vec3 dir);
        DirectionalLight(glm::vec3 dir, glm::vec3 a, glm::vec3 d, glm::vec3 s);

        glm::vec3 direction;
};

class PointLight : public Light {
    public:
        PointLight();
        PointLight(glm::vec3 pos);
        PointLight(glm::vec3 pos, glm::vec3 a, glm::vec3 d, glm::vec3 s);

        glm::vec3 position;
};
