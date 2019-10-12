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
    bool createGpuCopy   = true;
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
    void Free( bool gpuCopy = true, bool cpuCopy = true );
    
    bool LoadFromObj( ModelCreateInfo* createInfo );
    void RecalculateBB( bool recursive = false );
    void Optimize();

    AABB aabb;
    std::vector< Mesh > meshes;
    std::vector< std::shared_ptr< Material > > materials;
};

} // namespace Progression
