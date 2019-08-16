#pragma once

#include "resource/mesh.hpp"
#include "resource/resource.hpp"
#include "utils/noncopyable.hpp"
#include <vector>

namespace Progression
{

class Material;

struct ModelCreateInfo : public ResourceCreateInfo
{
    std::string filename = "";
    bool optimize        = true;
    bool freeCpuCopy     = true;
};

class Model : public Resource
{
public:
    Model() = default;
    Model( Model&& model );
    Model& operator=( Model&& model );

    bool Load( ResourceCreateInfo* createInfo ) override;
    void Move( std::shared_ptr< Resource > dst ) override;
    bool Serialize( std::ofstream& outFile ) const override;
    bool Deserialize( char*& buffer ) override;
    
    bool LoadFromObj( ModelCreateInfo* createInfo, bool loadTexturesIfNotInResourceManager = false );
    void RecalculateBB( bool recursive = false );
    void Optimize();

    AABB aabb;
    std::vector< Mesh > meshes;
    std::vector< std::shared_ptr< Material > > materials;
};

} // namespace Progression
