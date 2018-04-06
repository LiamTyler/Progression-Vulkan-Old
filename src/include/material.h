#pragma once

#include "include/utils.h"

class Material {
    public:
        Material();
        ~Material();
        Material(glm::vec3 a, glm::vec3 d, glm::vec3 s, float spec);
        
        glm::vec3 ka;
        glm::vec3 kd;
        glm::vec3 ks;
        float specular;
};
