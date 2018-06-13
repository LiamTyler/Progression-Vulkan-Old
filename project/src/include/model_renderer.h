#pragma once

#include "include/utils.h"
#include "include/mesh_renderer.h"
#include "include/model.h"

class ModelRenderer : public Component {
	public:
		ModelRenderer(Shader* sh, Model* m);
		~ModelRenderer();

		void Start();
		void Update(float dt);
		void Stop();
		void Render(const Camera& camera);

	protected:
		Shader* shader_;
        Model* model_;
        std::vector<MeshRenderer*> mesh_renderers_;
};
