#include "graphics/mesh_render_subsystem.h"
#include "graphics/shader.h"
#include "graphics/render_system.h"

namespace Progression {

    MeshRenderSubSystem::MeshRenderSubSystem() {

    }

    void MeshRenderSubSystem::AddRenderComponent(RenderComponent* rc) {
		std::cout << "in add RC " << std::endl;
        MeshRenderer* mr = static_cast<MeshRenderer*>(rc);
        meshRenderers.push_back(mr);
        assert(mr->mesh != nullptr);
        assert(mr->material != nullptr);
        assert(mr->material->shader != nullptr);
        GLuint vao;
		std::cout << "in add RC 2 " << std::endl;

        if (vaos_.find(mr->mesh) == vaos_.end()) {
			std::cout << "in add RC 3" << std::endl;

            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);
            GLuint* vbos_ = &mr->mesh->vbos[0];
            Shader& shader = *mr->material->shader;

            glBindBuffer(GL_ARRAY_BUFFER, vbos_[Mesh::vboName::VERTEX]);
            glEnableVertexAttribArray(shader["vertex"]);
            glVertexAttribPointer(shader["vertex"], 3, GL_FLOAT, GL_FALSE, 0, 0);

            glBindBuffer(GL_ARRAY_BUFFER, vbos_[Mesh::vboName::NORMAL]);
            glEnableVertexAttribArray(shader["normal"]);
            glVertexAttribPointer(shader["normal"], 3, GL_FLOAT, GL_FALSE, 0, 0);

            if (mr->mesh->vbos[Mesh::vboName::UV] != -1) {
                glBindBuffer(GL_ARRAY_BUFFER, vbos_[Mesh::vboName::UV]);
                glEnableVertexAttribArray(shader["inTexCoord"]);
                glVertexAttribPointer(shader["inTexCoord"], 2, GL_FLOAT, GL_FALSE, 0, 0);
            }

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos_[Mesh::vboName::INDEX]);
            vaos_[mr->mesh] = vao;
        } else {
			std::cout << "in add RC 4" << std::endl;

            vao = vaos_[mr->mesh];
        }

        mr->vao = vao;
    }
    
    // TODO: Implement
    void MeshRenderSubSystem::RemoveRenderComponent(RenderComponent* rc) {

    }

    void MeshRenderSubSystem::Render(Scene* scene, Camera& camera) {
        Shader* currShader = nullptr;
        for (const auto& mr : meshRenderers) {
            if (mr->material->shader != currShader) {
                currShader = mr->material->shader;
                currShader->Enable();
                RenderSystem::UploadCameraProjection(*currShader, camera);
                RenderSystem::UploadLights(*currShader);
            }
            Shader& shader = *currShader;
            glBindVertexArray(mr->vao);
            RenderSystem::UploadMaterial(shader, *mr->material);
            glm::mat4 M = mr->gameObject->transform.GetModelMatrix();
            glm::mat4 MV = camera.GetV() * M;
            glm::mat4 normalMatrix = glm::transpose(glm::inverse(MV));
            glUniformMatrix4fv(shader["modelViewMatrix"], 1, GL_FALSE, glm::value_ptr(MV));
            glUniformMatrix4fv(shader["normalMatrix"], 1, GL_FALSE, glm::value_ptr(normalMatrix));
            glDrawElements(GL_TRIANGLES, mr->mesh->numTriangles * 3, GL_UNSIGNED_INT, 0);
        }
    }

} // namespace Progression