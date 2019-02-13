#include "graphics/model_render_component.hpp"
#include "graphics/render_system.hpp"

namespace Progression {

    ModelRenderer::ModelRenderer(GameObject* go, Model* mod, bool _active) :
        RenderComponent(go, _active),
        model(mod)
    {
    }

    ModelRenderer::~ModelRenderer() {
    }

    void ModelRenderer::Start() {
    }

    void ModelRenderer::Update() {

    }

    void ModelRenderer::Stop() {

    }

} // namespace Progression
