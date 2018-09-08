#pragma once

#include "graphics/render_component.h"
#include "graphics/mesh_render_component.h"
#include "graphics/model.h"

namespace Progression {

    class ModelRenderer : public RenderComponent {
    public:
        ModelRenderer(GameObject* go, Model* model, bool active = true);
        virtual ~ModelRenderer();
        virtual void Start();
        virtual void Update();
        virtual void Stop();

        Model* model;
        std::vector<MeshRenderer*> meshRenderers;
    };
} // namespace Progression