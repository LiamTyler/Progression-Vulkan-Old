#pragma once

#include <vector>

namespace Progression {

    class Mesh;
    class Material;

    class Model {
    public:
        Model() = default;
		~Model() = default;

        std::vector<Mesh> meshes;
        std::vector<Material*> materials;
    };

} // namespace Progression
