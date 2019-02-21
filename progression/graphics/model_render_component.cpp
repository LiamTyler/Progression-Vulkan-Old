#include "graphics/model_render_component.hpp"
#include "graphics/render_system.hpp"

namespace Progression {

    ModelRenderer::ModelRenderer(GameObject* go, Model* mod, const std::vector<std::shared_ptr<Material>>& mats) :
        RenderComponent(go),
        model(mod),
        materials(mats)
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
