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

        std::vector<std::shared_ptr<Mesh>> meshes;
        std::vector<std::shared_ptr<Material>> materials;
    };

} // namespace Progression
