#include "graphics/mesh_render_subsystem.hpp"
#include "graphics/shader.hpp"
#include "graphics/render_system.hpp"
#include "core/resource_manager.hpp"
#include "utils/logger.hpp"

namespace Progression {

    MeshRenderSubSystem::MeshRenderSubSystem() {
        if (!pipelineShaders[RenderingPipeline::FORWARD].Load(PG_RESOURCE_DIR "shaders/default.vert", PG_RESOURCE_DIR "shaders/phong_forward.frag")) {
            LOG_ERR("Could not load the mesh renderer forward shader");
            exit(EXIT_FAILURE);
        }
        if (!pipelineShaders[RenderingPipeline::TILED_DEFERRED].Load(PG_RESOURCE_DIR "shaders/default.vert", PG_RESOURCE_DIR "shaders/deferred_save_gbuffer.frag")) {
            LOG_ERR("Could not load the mesh renderer tiled deferred shader");
            exit(EXIT_FAILURE);
        }
    }


    MeshRenderSubSystem::~MeshRenderSubSystem() {
        for (auto& pair : vaos_)
            glDeleteVertexArrays(1, &pair.second);
    }

    void MeshRenderSubSystem::AddRenderComponent(RenderComponent* rc) {
        MeshRenderer* mr = static_cast<MeshRenderer*>(rc);
        meshRenderers.push_back(mr);
        assert(mr->mesh != nullptr);
        assert(mr->material != nullptr);
        GLuint vao;

        if (vaos_.find(mr->mesh) == vaos_.end()) {
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);
            GLuint* vbos_ = &mr->mesh->vbos[0];
            Shader& shader = pipelineShaders[RenderingPipeline::FORWARD];

            glBindBuffer(GL_ARRAY_BUFFER, vbos_[Mesh::vboName::VERTEX]);
            glEnableVertexAttribArray(shader["vertex"]);
            glVertexAttribPointer(shader["vertex"], 3, GL_FLOAT, GL_FALSE, 0, 0);

            glBindBuffer(GL_ARRAY_BUFFER, vbos_[Mesh::vboName::NORMAL]);
            glEnableVertexAttribArray(shader["normal"]);
            glVertexAttribPointer(shader["normal"], 3, GL_FLOAT, GL_FALSE, 0, 0);

            if (mr->mesh->vbos[Mesh::vboName::UV] != (GLuint) -1) {
                glBindBuffer(GL_ARRAY_BUFFER, vbos_[Mesh::vboName::UV]);
                glEnableVertexAttribArray(shader["inTexCoord"]);
                glVertexAttribPointer(shader["inTexCoord"], 2, GL_FLOAT, GL_FALSE, 0, 0);
            }

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos_[Mesh::vboName::INDEX]);
            vaos_[mr->mesh] = vao;
        } else {
            vao = vaos_[mr->mesh];
        }

        mr->vao = vao;
    }
    
    // TODO: Implement
    void MeshRenderSubSystem::RemoveRenderComponent(RenderComponent* rc) {
        (void)rc;
    }

    /*void MeshRenderSubSystem::DepthPass(Shader& shader, const glm::mat4& LSM) {
        for (const auto& mr : meshRenderers) {
            glBindVertexArray(mr->vao);
            glm::mat4 M = mr->gameObject->transform.GetModelMatrix();
            glm::mat4 MVP = LSM * M;
            glUniformMatrix4fv(shader["MVP"], 1, GL_FALSE, glm::value_ptr(MVP));
            glDrawElements(GL_TRIANGLES, mr->mesh->getNumIndices(), GL_UNSIGNED_INT, 0);
        }
    }*/

    void MeshRenderSubSystem::DepthPass(Shader& shader, const glm::mat4& LSM) {
        for (const auto& mr : meshRenderers) {
            glBindVertexArray(mr->vao);
            glm::mat4 M = mr->gameObject->transform.GetModelMatrix();
            glm::mat4 MVP = LSM * M;
            glUniformMatrix4fv(shader["M"], 1, GL_FALSE, glm::value_ptr(MVP));
            glDrawElements(GL_TRIANGLES, mr->mesh->getNumIndices(), GL_UNSIGNED_INT, 0);
        }
    }

    void MeshRenderSubSystem::DepthRender(Shader& shader, const Camera& camera) {
        auto VP = camera.GetP() * camera.GetV();
        for (const auto& mr : meshRenderers) {
            glBindVertexArray(mr->vao);
            RenderSystem::UploadMaterial(shader, *mr->material);
            glm::mat4 M = mr->gameObject->transform.GetModelMatrix();
            glm::mat4 N = glm::transpose(glm::inverse(M));
            glm::mat4 MVP = VP * M;
            glUniformMatrix4fv(shader["M"], 1, GL_FALSE, glm::value_ptr(M));
            glUniformMatrix4fv(shader["N"], 1, GL_FALSE, glm::value_ptr(N));
            glUniformMatrix4fv(shader["MVP"], 1, GL_FALSE, glm::value_ptr(MVP));
            glDrawElements(GL_TRIANGLES, mr->mesh->getNumIndices(), GL_UNSIGNED_INT, 0);
        }
    }


    void MeshRenderSubSystem::Render(const Camera& camera) {
		auto& shader = pipelineShaders[camera.GetRenderingPipeline()];
		shader.Enable();
		RenderSystem::UploadLights(shader);
        glUniform3fv(shader["cameraPos"], 1, glm::value_ptr(camera.transform.position));
        auto VP = camera.GetP() * camera.GetV();
        for (const auto& mr : meshRenderers) {
            glBindVertexArray(mr->vao);
            RenderSystem::UploadMaterial(shader, *mr->material);
            glm::mat4 M   = mr->gameObject->transform.GetModelMatrix();
            glm::mat4 N   = glm::transpose(glm::inverse(M));
            glm::mat4 MVP = VP * M;
            glUniformMatrix4fv(shader["M"], 1, GL_FALSE, glm::value_ptr(M));
            glUniformMatrix4fv(shader["N"], 1, GL_FALSE, glm::value_ptr(N));
            glUniformMatrix4fv(shader["MVP"], 1, GL_FALSE, glm::value_ptr(MVP));
            glDrawElements(GL_TRIANGLES, mr->mesh->getNumIndices(), GL_UNSIGNED_INT, 0);
        }
    }

} // namespace Progression
