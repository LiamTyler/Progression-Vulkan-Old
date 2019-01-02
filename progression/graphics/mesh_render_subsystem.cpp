#include "graphics/mesh_render_subsystem.hpp"
#include "graphics/shader.hpp"
#include "graphics/render_system.hpp"
#include "core/resource_manager.hpp"

namespace Progression {

    MeshRenderSubSystem::MeshRenderSubSystem() {
        auto forward = ResourceManager::GetShader("mesh-forward");
        auto tiled_deferred = ResourceManager::GetShader("mesh-tiled-deferred");
        if (!forward) {
            forward = ResourceManager::AddShader(
                    Shader(PG_RESOURCE_DIR "shaders/phong.vert", PG_RESOURCE_DIR "shaders/phong_forward.frag"),
                    "mesh-forward");
            // forward->AddUniform("lights");
        }
        if (!tiled_deferred) {
            tiled_deferred = ResourceManager::AddShader(
                    Shader(PG_RESOURCE_DIR "shaders/phong.vert", PG_RESOURCE_DIR "shaders/phong_tiled_deferred.frag"),
                    "mesh-tiled-deferred");
        }

        pipelineShaders[RenderingPipeline::FORWARD] = forward.get();
        pipelineShaders[RenderingPipeline::TILED_DEFERRED] = tiled_deferred.get();
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
            Shader& shader = *pipelineShaders[RenderingPipeline::FORWARD];

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
            vao = vaos_[mr->mesh];
        }

        mr->vao = vao;
    }
    
    // TODO: Implement
    void MeshRenderSubSystem::RemoveRenderComponent(RenderComponent* rc) {

    }

    void MeshRenderSubSystem::Render(Scene* scene, Camera& camera) {
		auto& shader = *pipelineShaders[camera.GetRenderingPipeline()];
		shader.Enable();
		if (camera.GetRenderingPipeline() == RenderingPipeline::FORWARD)
			RenderSystem::UploadLights(shader);
        RenderSystem::UploadCameraProjection(shader, camera);
        for (const auto& mr : meshRenderers) {
            glBindVertexArray(mr->vao);
            RenderSystem::UploadMaterial(shader, *mr->material);
            glm::mat4 M = mr->gameObject->transform.GetModelMatrix();
            glm::mat4 MV = camera.GetV() * M;
            glm::mat4 normalMatrix = glm::transpose(glm::inverse(MV));
            glUniformMatrix4fv(shader["modelViewMatrix"], 1, GL_FALSE, glm::value_ptr(MV));
            glUniformMatrix4fv(shader["normalMatrix"], 1, GL_FALSE, glm::value_ptr(normalMatrix));
            glDrawElements(GL_TRIANGLES, mr->mesh->getNumIndices(), GL_UNSIGNED_INT, 0);
        }
    }

} // namespace Progression
