#pragma once

#include "graphics/render_component.hpp"
#include "graphics/model.hpp"
#include "graphics/material.hpp"
#include <memory>

namespace Progression {

    class ModelRenderer : public RenderComponent {
    public:
        ModelRenderer(GameObject* go, Model* model, const std::vector<std::shared_ptr<Material>>& materials);
        virtual ~ModelRenderer();
        virtual void Start();
        virtual void Update();
        virtual void Stop();

        Model* model;
        std::vector<std::shared_ptr<Material>> materials;
    };

} // namespace Progression
