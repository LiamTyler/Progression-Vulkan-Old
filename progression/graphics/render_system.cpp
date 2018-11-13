#include "graphics/render_system.h"
#include "graphics/render_subsystem.h"
#include "graphics/graphics_api.h"
#include "core/scene.h"
#include "core/camera.h"
#include "graphics/mesh_render_subsystem.h"

namespace Progression {

    std::unordered_map<std::type_index, RenderSubSystem*> RenderSystem::subSystems_ = {};
    std::vector<glm::vec3> RenderSystem::lightBuffer_ = {};
    unsigned int RenderSystem::numDirectionalLights_ = 0;
    unsigned int RenderSystem::numPointLights_ = 0;
    GLuint RenderSystem::lightSSBO_ = 0;
    unsigned int RenderSystem::maxNumLights_ = 0;
    float RenderSystem::lightIntensityCutoff_ = 0;

    void RenderSystem::Init(const config::Config& config) {
        // auto load the mesh renderer subsystem
        subSystems_[typeid(MeshRenderSubSystem)] = new MeshRenderSubSystem;

        auto rsConfig = config->get_table("renderSystem");
        if (!rsConfig) {
            std::cout << "Need to specify the 'renderSystem' in the config file!" << std::endl;
            exit(0);
        }
        maxNumLights_ = rsConfig->get_as<int>("maxNumLights").value_or(10001);
        lightIntensityCutoff_ = rsConfig->get_as<float>("lightIntensityCutoff").value_or(0.03f);

        glGenBuffers(1, &lightSSBO_);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO_);
        glBufferData(GL_SHADER_STORAGE_BUFFER, 2 * sizeof(glm::vec4) * maxNumLights_, NULL, GL_STREAM_COPY);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, lightSSBO_);
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
		// glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        graphics::ToggleDepthTesting(true);
        // graphics::ToggleCulling(true);

        for (const auto& subsys : subSystems_)
            subsys.second->Render(scene, *camera);
    }

    void RenderSystem::UpdateLights(Scene* scene, Camera* camera) {
       
        /*
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
        */

        GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
        numPointLights_ = 0;
        numDirectionalLights_ = 0;

        const auto& dirLights = scene->GetDirectionalLights();
        const auto& pointLights = scene->GetPointLights();
        glm::mat4 V = camera->GetV();
        
        glm::vec4* lightBuffer = (glm::vec4*) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 2 * sizeof(glm::vec4) * maxNumLights_, bufMask);
        int i = 0;
        for (i = 0; i < dirLights.size() && i < maxNumLights_; ++i) {
            glm::vec3 dir(0, 0, -1);
            glm::mat4 rot(1);
            rot = glm::rotate(rot, dirLights[i]->transform.rotation.z, glm::vec3(0, 0, 1));
            rot = glm::rotate(rot, dirLights[i]->transform.rotation.y, glm::vec3(0, 1, 0));
            rot = glm::rotate(rot, dirLights[i]->transform.rotation.x, glm::vec3(1, 0, 0));
            lightBuffer[2 * i + 0] = V * rot * glm::vec4(dir, 0);
            lightBuffer[2 * i + 1] = glm::vec4(dirLights[i]->intensity * dirLights[i]->color, 1);
        }
        numDirectionalLights_ = i;

        for (int i = 0; i < pointLights.size() && (i + dirLights.size()) < maxNumLights_; ++i) {
            float lightRadius = sqrtf(pointLights[i]->intensity / lightIntensityCutoff_);
            lightBuffer[2 * (numDirectionalLights_ + i) + 0] = V * glm::vec4(pointLights[i]->transform.position, 1);
            lightBuffer[2 * (numDirectionalLights_ + i) + 0].w = lightRadius;
            lightBuffer[2 * (numDirectionalLights_ + i) + 1] = glm::vec4(pointLights[i]->intensity * pointLights[i]->color, 1);
        }
        numPointLights_ = i;

        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }


     void RenderSystem::UploadLights(Shader& shader) {
         glUniform1i(shader["numDirectionalLights"], numDirectionalLights_);
         glUniform1i(shader["numPointLights"], numPointLights_);
         /*
         if (numDirectionalLights_ + numPointLights_ > 0) {
             glUniform3fv(shader["lights"], 2 * (numDirectionalLights_ + numPointLights_), &lightBuffer_[0].x);
         }
         */
     }

     void RenderSystem::UploadCameraProjection(Shader& shader, Camera& camera) {
         glUniformMatrix4fv(shader["projectionMatrix"], 1, GL_FALSE, glm::value_ptr(camera.GetP()));
     }

     void RenderSystem::UploadMaterial(Shader& shader, Material& material) {
         glUniform3fv(shader["ka"], 1, glm::value_ptr(material.ambient));
         glUniform3fv(shader["kd"], 1, glm::value_ptr(material.diffuse));
		 glUniform3fv(shader["ks"], 1, glm::value_ptr(material.specular));
		 glUniform3fv(shader["ke"], 1, glm::value_ptr(material.emissive));
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
