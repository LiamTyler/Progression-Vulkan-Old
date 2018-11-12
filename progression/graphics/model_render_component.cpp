#include "graphics/model_render_component.h"
#include "graphics/render_system.h"
#include "graphics/mesh_render_subsystem.h"
#include "graphics/mesh_render_component.h"

namespace Progression {

    ModelRenderer::ModelRenderer(GameObject* go, Model* mod, bool active) :
        RenderComponent(go, active),
        model(mod)
    {
        for (int i = 0; i < model->meshes.size(); i++) {
            MeshRenderer* mr = new MeshRenderer(go, model->meshes[i].get(), model->materials[i].get());
            meshRenderers.push_back(mr);
        }
    }

    // TODO: clean up
    ModelRenderer::~ModelRenderer() {
            
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