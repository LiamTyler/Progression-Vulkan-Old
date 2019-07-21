#include "components/model_renderer_component.hpp"
#include "resource/model.hpp"
#include "resource/material.hpp"
#include "utils/fileIO.hpp"
#include "resource/resource_manager.hpp"
#include "utils/logger.hpp"

namespace Progression {

    void parseModelRendererComponentFromFile(std::istream& in, ModelRenderComponent& comp) {
        std::string tmp;
        fileIO::parseLineKeyVal(in, "model", tmp);
        comp.model = ResourceManager::get<Model>(tmp);
        PG_ASSERT(comp.model);
        comp.materials = comp.model->materials;
        int numMaterials;
        fileIO::parseLineKeyVal(in, "CustomMaterials", numMaterials);
        for (int i = 0; i < numMaterials; ++i) {
            std::string line;
            std::getline(in, line);
            std::stringstream ss(line);
            uint32_t materialIndex;
            ss >> materialIndex >> tmp;
            auto mat = ResourceManager::get<Material>(tmp);
            PG_ASSERT(!ss.fail() && mat && materialIndex < comp.materials.size());
            comp.materials[materialIndex] = mat;
        }

        PG_ASSERT(!in.fail());
    }

} // namespace Progression