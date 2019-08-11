#pragma once

#include "core/common.hpp"
#include "resource/resource.hpp"

namespace Progression
{

class Texture;

class MaterialCreateInfo : public ResourceCreateInfo
{
public:
    glm::vec3 Ka;
    glm::vec3 Kd;
    glm::vec3 Ks;
    glm::vec3 Ke;
    float Ns;
    std::string map_Kd_name;
};

class Material : public Resource
{
public:
    Material() = default;

    bool Load( ResourceCreateInfo* createInfo ) override;
    void Move( std::shared_ptr< Resource > dst ) override;
    bool Serialize( std::ofstream& outFile ) const override;
    bool Deserialize( char*& buffer ) override;
    static bool LoadMtlFile( std::vector< Material >& materials, const std::string& fname );

    glm::vec3 Ka;
    glm::vec3 Kd;
    glm::vec3 Ks;
    glm::vec3 Ke;
    float Ns;
    std::shared_ptr<Texture> map_Kd = nullptr;
};

} // namespace Progression
