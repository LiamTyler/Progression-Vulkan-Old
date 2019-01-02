#include "graphics/mesh_render_component.hpp"

namespace Progression {

    MeshRenderer::MeshRenderer(GameObject* go, Mesh* _mesh, Material* mat, bool active) :
        RenderComponent(go, active),
        mesh(_mesh),
        material(mat),
        vao(-1)
    {
    }

    // TODO: Clean up
    MeshRenderer::~MeshRenderer() {

    }

    void MeshRenderer::Start() {

    }

    void MeshRenderer::Update() {

    }

    void MeshRenderer::Stop() {

    }

} // namespace Progression