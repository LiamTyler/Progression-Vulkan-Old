#pragma once

#include <vector>

namespace Progression {
    class Mesh;
    class Material;

    class Model {
    public:
        Model() = default;
        virtual ~Model();

        // TODO: Fix model and mesh copy & move ctors
        Model(const Model& mesh) = delete;
        Model& operator=(const Model& mesh) = delete;
        Model(Model&& mesh) = default;
        Model& operator=(Model&& mesh) = default;

        std::vector<std::pair<Mesh*, Material*>> meshMaterialPairs;
    };

} // namespace Progression