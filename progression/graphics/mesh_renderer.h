#pragma once

#include "graphics/render_component.h"
#include "graphics/material.h"
#include "graphics/mesh.h"

namespace Progression {

    class MeshRenderer : public RenderComponent {
    public:
        MeshRenderer(GameObject* go, bool active = true);
        virtual ~MeshRenderer();
        virtual void Start();
        virtual void Update();
        virtual void Stop();

        Mesh* mesh;
        Material* material;
    };
} // namespace Progression