#include "include/material.h"

Material::Material() : Material(
        glm::vec3(0, 0, 0),
        glm::vec3(0, 0, 0),
        glm::vec3(0, 0, 0),
        0)
{
}

Material::~Material() {
}

Material::Material(glm::vec3 a, glm::vec3 d, glm::vec3 s,
        float spec) {
    ka = a;
    kd = d;
    ks = s;
    specular = spec;
}
