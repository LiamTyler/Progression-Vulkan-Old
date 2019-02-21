#pragma once

#include <vector>
#include <memory>

namespace Progression {

    class Mesh;

    class Model {
    public:
        Model() = default;
		virtual ~Model() = default;

        std::vector<std::shared_ptr<Mesh>> meshes;
    };

} // namespace Progression
