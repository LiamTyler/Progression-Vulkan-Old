#pragma once

#include "core/math.hpp"
#include "resource/image.hpp"
#include <vector>

namespace Progression
{

class MaterialCreateInfo : public ResourceCreateInfo
{
public:
    glm::vec3 Kd;
    float roughness;
    float metallic;
    bool transparent = false;
    std::string map_Kd_name;
    std::string map_Norm_name;
    std::string map_Pm_name;
    std::string map_Pr_name;
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

    glm::vec3 Kd;
    float roughness;
    float metallic;
    bool transparent = false;
    std::shared_ptr< Image > map_Kd   = nullptr;
    std::shared_ptr< Image > map_Norm = nullptr;
    std::shared_ptr< Image > map_Pm   = nullptr; // metallic
    std::shared_ptr< Image > map_Pr   = nullptr; // roughness
};

} // namespace Progression
