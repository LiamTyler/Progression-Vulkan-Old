#pragma once

#include <vector>
#include <memory>
#include <iostream>

namespace Progression {

    class Model;
    class Material;

    struct ModelRenderComponent {
        std::shared_ptr<Model> model;
        std::vector<std::shared_ptr<Material>> materials;
    };

    void parseModelRendererComponentFromFile(std::istream& in, ModelRenderComponent& comp);

} // namespace Progression