#include "graphics/render_system.h"
#include "graphics/render_subsystem.h"
#include "core/scene.h"
#include "core/camera.h"
#include "graphics/mesh_render_subsystem.h"

namespace Progression {

    std::unordered_map<std::type_index, RenderSubSystem*> RenderSystem::subSystems_ = {};
    std::vector<glm::vec3> RenderSystem::lightBuffer_ = {};
    unsigned int RenderSystem::numDirectionalLights_ = 0;
    unsigned int RenderSystem::numPointLights_ = 0;

    void RenderSystem::Init(const config::Config& config) {
        // auto load the mesh renderer subsystem
        subSystems_[typeid(MeshRenderSubSystem)] = new MeshRenderSubSystem;
    }

    void RenderSystem::Free() {
        for (auto& subsys : subSystems_)
            delete subsys.second;
    }

    void RenderSystem::Render(Scene* scene, Camera* camera) {
        if (!camera) {
            camera = scene->GetCamera(0);
        }

        // update light buffer
        UpdateLights(scene, camera);
        
        const auto& bgColor = scene->GetBackgroundColor();
        glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        for (const auto& subsys : subSystems_)
            subsys.second->Render(scene, *camera);
    }

    void RenderSystem::UpdateLights(Scene* scene, Camera* camera) {
        
        numPointLights_ = scene->GetNumPointLights();
        numDirectionalLights_ = scene->GetNumDirectionalLights();
        lightBuffer_.clear();
        lightBuffer_.resize(2 * (numDirectionalLights_ + numPointLights_));
        
        glm::mat4 V = camera->GetV();
        const auto& dirLights = scene->GetDirectionalLights();

        for (int i = 0; i < numDirectionalLights_; ++i) {
            glm::vec3 dir(0, 0, -1);
            glm::mat4 rot(1);
            rot = glm::rotate(rot, dirLights[i]->transform.rotation.z, glm::vec3(0, 0, 1));
            rot = glm::rotate(rot, dirLights[i]->transform.rotation.y, glm::vec3(0, 1, 0));
            rot = glm::rotate(rot, dirLights[i]->transform.rotation.x, glm::vec3(1, 0, 0));
            lightBuffer_[2 * i + 0] = glm::vec3(V * rot * glm::vec4(dir, 0));
            lightBuffer_[2 * i + 1] = dirLights[i]->intensity * dirLights[i]->color;
        }

        const auto& pointLights = scene->GetPointLights();
        for (int i = 0; i < numPointLights_; ++i) {
            lightBuffer_[2 * (numDirectionalLights_ + i) + 0] = glm::vec3(V * glm::vec4(pointLights[i]->transform.position, 1));
            lightBuffer_[2 * (numDirectionalLights_ + i) + 1] = pointLights[i]->intensity * pointLights[i]->color;
        }
    }


     void RenderSystem::UploadLights(Shader& shader) {
         glUniform1i(shader["numDirectionalLights"], numDirectionalLights_);
         glUniform1i(shader["numPointLights"], numPointLights_);
         if (numDirectionalLights_ + numPointLights_ > 0) {
             glUniform3fv(shader["lights"], 2 * (numDirectionalLights_ + numPointLights_), &lightBuffer_[0].x);
         }
     }

     void RenderSystem::UploadCameraProjection(Shader& shader, Camera& camera) {
         glUniformMatrix4fv(shader["projectionMatrix"], 1, GL_FALSE, glm::value_ptr(camera.GetP()));
     }

     void RenderSystem::UploadMaterial(Shader& shader, Material& material) {
         glUniform3fv(shader["ka"], 1, glm::value_ptr(material.ambient));
         glUniform3fv(shader["kd"], 1, glm::value_ptr(material.diffuse));
         glUniform3fv(shader["ks"], 1, glm::value_ptr(material.specular));
         glUniform1f(shader["specular"], material.shininess);
         if (material.diffuseTexture) {
             glUniform1i(shader["textured"], true);
             glActiveTexture(GL_TEXTURE0);
             glBindTexture(GL_TEXTURE_2D, material.diffuseTexture->getGPUHandle());
             glUniform1i(shader["diffuseTex"], 0);
         } else {
             glUniform1i(shader["textured"], false);
         }
     }

} // namespace Progression