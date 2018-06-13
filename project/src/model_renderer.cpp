#include "include/model_renderer.h"

ModelRenderer::ModelRenderer(Shader* sh, Model* m) {
    shader_ = sh;
    model_ = m;
}

ModelRenderer::~ModelRenderer() {
}

void ModelRenderer::Start() {
    for (auto& mesh : model_->GetMeshes()) {
        MeshRenderer* rc = new MeshRenderer(shader_, mesh);
        rc->gameObject = this->gameObject;
        rc->Start();
        mesh_renderers_.push_back(rc);
    }
}

void ModelRenderer::Update(float dt) {
}

void ModelRenderer::Stop() {
    for (auto& mr : mesh_renderers_) {
        mr->Stop();
        delete mr;
    }
}

void ModelRenderer::Render(const Camera& camera) {
    for (auto& rc : mesh_renderers_) {
        rc->Render(camera);
    }
}
