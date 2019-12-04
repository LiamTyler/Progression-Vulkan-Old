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
    glm::vec3 Ks;
    float Ns;
    std::string map_Kd_name;
    std::string map_Norm_name;
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
    glm::vec3 Ks;
    float Ns;
    std::shared_ptr< Image > map_Kd   = nullptr;
    std::shared_ptr< Image > map_Norm = nullptr;
};

} // namespace Progression
