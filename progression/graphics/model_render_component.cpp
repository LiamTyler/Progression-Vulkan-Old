#include "graphics/model_render_component.hpp"
#include "graphics/render_system.hpp"
#include "graphics/mesh_render_subsystem.hpp"
#include "graphics/mesh_render_component.hpp"

namespace Progression {

    ModelRenderer::ModelRenderer(GameObject* go, Model* mod, bool active) :
        RenderComponent(go, active),
        model(mod)
    {
        for (size_t i = 0; i < model->meshes.size(); i++) {
            MeshRenderer* mr = new MeshRenderer(go, model->meshes[i].get(), model->materials[i].get());
            meshRenderers.push_back(mr);
        }
    }

    ModelRenderer::~ModelRenderer() {
        for (auto& mr : meshRenderers) {
            delete mr;
        }
    }

    void ModelRenderer::Start() {
        auto subsys = RenderSystem::GetSubSystem<MeshRenderSubSystem>();
        for (const auto& meshR : meshRenderers)
            subsys->AddRenderComponent(meshR);
    }

    void ModelRenderer::Update() {

    }

    // TODO: detach from mesh render subsys?
    void ModelRenderer::Stop() {

    }

} // namespace Progression
