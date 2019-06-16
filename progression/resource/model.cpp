#include "resource/model.hpp"

namespace Progression {

    Model::Model(Model&& model) {
        *this = std::move(model);
    }

    Model& Model::operator=(Model&& model) {
        meshes = std::move(model.meshes);
        materials = std::move(model.materials);

        return *this;
    }

} // namespace Progression
