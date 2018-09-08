#include "graphics/mesh_render_subsystem.h"
#include "graphics/shader.h"
#include "graphics/render_system.h"

namespace Progression {

    MeshRenderSubSystem::MeshRenderSubSystem() {

    }

    void MeshRenderSubSystem::AddRenderComponent(RenderComponent* rc) {
        MeshRenderer* mr = static_cast<MeshRenderer*>(rc);
        meshRenderers.push_back(mr);
        assert(mr->mesh != nullptr);
        assert(mr->material != nullptr);
        assert(mr->material->shader != nullptr);
        GLuint vao;
        if (vaos_.find(mr->mesh) == vaos_.end()) {
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);
            GLuint* vbos_ = mr->mesh->getBuffers();
            Shader& shader = *mr->material->shader;

            glBindBuffer(GL_ARRAY_BUFFER, vbos_[0]);
            glEnableVertexAttribArray(shader["vertex"]);
            glVertexAttribPointer(shader["vertex"], 3, GL_FLOAT, GL_FALSE, 0, 0);

            glBindBuffer(GL_ARRAY_BUFFER, vbos_[1]);
            glEnableVertexAttribArray(shader["normal"]);
            glVertexAttribPointer(shader["normal"], 3, GL_FLOAT, GL_FALSE, 0, 0);

            glBindBuffer(GL_ARRAY_BUFFER, vbos_[2]);
            glEnableVertexAttribArray(shader["inTexCoord"]);
            glVertexAttribPointer(shader["inTexCoord"], 2, GL_FLOAT, GL_FALSE, 0, 0);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos_[3]);
            vaos_[mr->mesh] = vao;
        } else {
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
            glDrawElements(GL_TRIANGLES, mr->mesh->getNumTriangles() * 3, GL_UNSIGNED_INT, 0);
        }
    }

} // namespace Progression