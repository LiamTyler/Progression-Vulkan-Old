#pragma once

#include <vector>
#include <memory>

namespace Progression {
    class Mesh;
    class Material;

    class Model {
    public:
        Model() = default;
        virtual ~Model() = default;

        // TODO: Fix model and mesh copy & move ctors
        Model(const Model& mesh) = default;
        Model& operator=(const Model& mesh) = default;
        Model(Model&& mesh) = delete;
        Model& operator=(Model&& mesh) = delete;

        std::vector<std::shared_ptr<Mesh>> meshes;
        std::vector<std::shared_ptr<Material>> materials;
    };

} // namespace Progression