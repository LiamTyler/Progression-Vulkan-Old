#pragma once

#include "graphics/render_component.hpp"
#include "graphics/model.hpp"

namespace Progression {

    class ModelRenderer : public RenderComponent {
    public:
        ModelRenderer(GameObject* go, Model* model, bool active = true);
        virtual ~ModelRenderer();
        virtual void Start();
        virtual void Update();
        virtual void Stop();

        Model* model;
    };

} // namespace Progression
