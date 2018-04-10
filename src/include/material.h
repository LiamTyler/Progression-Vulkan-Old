#pragma once

#include "include/utils.h"
#include "include/texture.h"

class Material {
    public:
        Material();
        ~Material();
        Material(glm::vec3 a, glm::vec3 d, glm::vec3 s, float spec);
		friend std::ostream& operator<<(std::ostream& out, const Material& mat);
        
        glm::vec3 ka;
        glm::vec3 kd;
        glm::vec3 ks;
        float specular;
		Texture* diffuseTex;
};
