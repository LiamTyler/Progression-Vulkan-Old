#include "include/mesh_renderer.h"

#include "include/mesh.h"
#include "include/shader.h"
#include "include/camera.h"
#include "include/game_object.h"

namespace Progression {

	MeshRenderer::MeshRenderer(Shader* sh, Mesh* m) {
		shader_ = sh;
		mesh_ = m;
	}

	void MeshRenderer::Start() {
		Shader& shader = *shader_;

		glGenVertexArrays(1, &vao_);
		glBindVertexArray(vao_);
		vbos_.resize(4);
		glGenBuffers(4, &vbos_[0]);
		glBindBuffer(GL_ARRAY_BUFFER, vbos_[0]);
		glBufferData(GL_ARRAY_BUFFER, mesh_->numVertices * sizeof(glm::vec3),
			mesh_->vertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(shader["vertex"]);
		glVertexAttribPointer(shader["vertex"], 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, vbos_[1]);
		glBufferData(GL_ARRAY_BUFFER, mesh_->numVertices * sizeof(glm::vec3),
			mesh_->normals, GL_STATIC_DRAW);
		glEnableVertexAttribArray(shader["normal"]);
		glVertexAttribPointer(shader["normal"], 3, GL_FLOAT, GL_FALSE, 0, 0);

		if (mesh_->HasTextureCoords()) {
			glBindBuffer(GL_ARRAY_BUFFER, vbos_[2]);
			glBufferData(GL_ARRAY_BUFFER, mesh_->numVertices * sizeof(glm::vec2),
				mesh_->texCoords, GL_STATIC_DRAW);
			glEnableVertexAttribArray(shader["inTexCoord"]);
			glVertexAttribPointer(shader["inTexCoord"], 2, GL_FLOAT, GL_FALSE, 0, 0);
		}
		else {
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos_[3]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh_->numTriangles * sizeof(glm::ivec3),
			mesh_->indices, GL_STATIC_DRAW);

		textured_ = mesh_->material->diffuseTex != nullptr;
	}

	void MeshRenderer::Update(float dt) {
	}

	void MeshRenderer::Stop() {
		glDeleteBuffers(static_cast<GLsizei>(vbos_.size()), &vbos_[0]);
		glDeleteVertexArrays(1, &vao_);
	}

	void MeshRenderer::Render(const Camera& camera) {
		Shader& shader = *shader_;
		glBindVertexArray(vao_);
		// send material
		glUniform3fv(shader["ka"], 1, glm::value_ptr(mesh_->material->ka));
		glUniform3fv(shader["kd"], 1, glm::value_ptr(mesh_->material->kd));
		glUniform3fv(shader["ks"], 1, glm::value_ptr(mesh_->material->ks));
		glUniform1f(shader["specular"], mesh_->material->specular);
		if (textured_) {
			glUniform1i(shader["textured"], true);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, mesh_->material->diffuseTex->GetHandle());
			glUniform1i(shader["diffuseTex"], 0);
		}
		else {
			glUniform1i(shader["textured"], false);
		}

		// send model and normal matrices
		glm::mat4 modelMatrix = gameObject->transform.GetModelMatrix();
		glm::mat4 MV = camera.GetV() * modelMatrix;
		glm::mat4 normalMatrix = glm::transpose(glm::inverse(MV));
		glUniformMatrix4fv(shader["modelViewMatrix"], 1, GL_FALSE, glm::value_ptr(MV));
		glUniformMatrix4fv(shader["normalMatrix"], 1, GL_FALSE, glm::value_ptr(normalMatrix));

		// draw
		glDrawElements(GL_TRIANGLES, mesh_->numTriangles * 3, GL_UNSIGNED_INT, 0);
	}

} // namespace Progression
