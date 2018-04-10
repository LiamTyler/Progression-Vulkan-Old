#pragma once

#include "include/utils.h"
#include "include/component.h"
#include "include/game_object.h"
#include "include/mesh.h"
#include "include/shader.h"
#include "include/camera.h"

class MeshRenderer : public Component {
	public:
		MeshRenderer(Shader* sh, Mesh* m);
		~MeshRenderer();

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
