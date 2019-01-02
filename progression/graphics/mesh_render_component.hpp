#pragma once

#include "graphics/render_component.hpp"
#include "graphics/material.hpp"
#include "graphics/mesh.hpp"

namespace Progression {

    class MeshRenderer : public RenderComponent {
    public:
        MeshRenderer(GameObject* go, Mesh* mesh = nullptr, Material* mat = nullptr, bool active = true);
        virtual ~MeshRenderer();
        virtual void Start();
        virtual void Update();
        virtual void Stop();

        Mesh* mesh;
        Material* material;
        GLuint vao;
    };
} // namespace Progression