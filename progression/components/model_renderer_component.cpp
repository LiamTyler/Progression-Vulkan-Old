#include "components/model_renderer_component.hpp"
#include "resource/material.hpp"
#include "resource/model.hpp"
#include "resource/resource_manager.hpp"
#include "utils/fileIO.hpp"
#include "utils/logger.hpp"

namespace Progression
{

void ParseModelRendererComponentFromFile( std::istream& in, ModelRenderComponent& comp )
{
    std::string tmp;
    fileIO::ParseLineKeyVal( in, "model", tmp );
    comp.model = ResourceManager::Get< Model >( tmp );
    PG_ASSERT( comp.model );
    comp.materials = comp.model->materials;
    int numMaterials;
    fileIO::ParseLineKeyVal( in, "CustomMaterials", numMaterials );
    for ( int i = 0; i < numMaterials; ++i )
    {
        std::string line;
        std::getline( in, line );
        std::stringstream ss( line );
        uint32_t materialIndex;
        ss >> materialIndex >> tmp;
        auto mat = ResourceManager::Get< Material >( tmp );
        PG_ASSERT( !ss.fail() && mat && materialIndex < comp.materials.size() );
        comp.materials[materialIndex] = mat;
    }

    PG_ASSERT( !in.fail() );
}

} // namespace Progression