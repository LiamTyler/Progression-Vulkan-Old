#pragma once

#include <vector>

#include "include/utils.h"
#include "include/shader.h"
#include "include/mesh.h"
#include "include/camera.h"
#include "include/component.h"

namespace Progression {

	class MeshRenderer : public Component {
	public:
		MeshRenderer(Shader* sh, Mesh* m);
		~MeshRenderer() = default;

		void Start();
		void Update(float dt);
		void Stop();
		void Render(const Camera& camera);

	protected:
		Shader* shader_;
		Mesh* mesh_;
		GLuint vao_;
		std::vector<GLuint> vbos_;
		bool textured_;
	};

} // namespace Progression