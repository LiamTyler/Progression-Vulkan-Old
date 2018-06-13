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
    diffuseTex = nullptr;
}

std::ostream& operator<<(std::ostream& out, const Material& mat) {
    out << "ka: " << mat.ka << std::endl;
    out << "kd: " << mat.kd << std::endl;
    out << "ks: " << mat.ks << std::endl;
    out << "specular: " << mat.specular << std::endl;
    if (mat.diffuseTex) {
        out << "diffuse tex: " << mat.diffuseTex->GetName() << std::endl;
    } else {
        out << "diffuse tex: None" << std::endl;
    }
    return out;
}
