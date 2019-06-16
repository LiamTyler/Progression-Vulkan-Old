#pragma once

#include <vector>
#include "resource/mesh.hpp"
#include "utils/noncopyable.hpp"

namespace Progression {

    class Material;

    class Model : public NonCopyable {
    public:
        Model() = default;
        Model(Model&& model);
        Model& operator=(Model&& model);

        std::vector<Mesh> meshes;
        std::vector<Material*> materials;
    };

} // namespace Progression
