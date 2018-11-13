#include "graphics/render_system.h"
#include "graphics/render_subsystem.h"
#include "graphics/graphics_api.h"
#include "core/scene.h"
#include "core/camera.h"
#include "graphics/mesh_render_subsystem.h"
#include "core/window.h"

namespace Progression {

    std::unordered_map<std::type_index, RenderSubSystem*> RenderSystem::subSystems_ = {};
    std::vector<glm::vec3> RenderSystem::lightBuffer_ = {};
    unsigned int RenderSystem::numDirectionalLights_ = 0;
    unsigned int RenderSystem::numPointLights_ = 0;
    GLuint RenderSystem::lightSSBO_ = 0;
    unsigned int RenderSystem::maxNumLights_ = 0;
    float RenderSystem::lightIntensityCutoff_ = 0;
    GLuint RenderSystem::tdGbuffer_ = 0;
    GLuint RenderSystem::tdTextures_[4] = {0, 0, 0, 0};
    GLuint RenderSystem::tdDepth_ = 0;
    Shader* RenderSystem::tdLightingShader_ = nullptr;
    GLuint RenderSystem::tdLightingOutput_ = 0;
    Shader* RenderSystem::tdCombineShader_ = nullptr;
    GLuint RenderSystem::tdQuadVAO_ = 0;
    GLuint RenderSystem::tdQuadVBO_ = 0;

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
		std::cout << "max Lights = " << maxNumLights_ << std::endl;
		std::cout << "lightIntensityCutoff_ = " << lightIntensityCutoff_ << std::endl;
		std::cout << "window size = " << Window::getWindowSize().x << " " << Window::getWindowSize().y << std::endl;

		tdCombineShader_ = new Shader(PG_RESOURCE_DIR "shaders/deferred_combine.vert", PG_RESOURCE_DIR "shaders/deferred_combine.frag");

		float quadVerts[] = {
			-1, 1,
			-1, -1,
			1, -1,

			-1, 1,
			1, -1,
			1, 1
		};
		glGenVertexArrays(1, &tdQuadVAO_);
		glBindVertexArray(tdQuadVAO_);
		glGenBuffers(1, &tdQuadVBO_);
		glBindBuffer(GL_ARRAY_BUFFER, tdQuadVBO_);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
		glEnableVertexAttribArray((*tdCombineShader_)["vertex"]);
		glVertexAttribPointer((*tdCombineShader_)["vertex"], 2, GL_FLOAT, GL_FALSE, 0, 0);

        tdGbuffer_     = graphics::CreateFrameBuffer();
        tdTextures_[0] = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA32F);
        tdTextures_[1] = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA32F);
        tdTextures_[2] = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA);
        tdTextures_[3] = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA32F);

        graphics::AttachColorTexturesToFBO({ tdTextures_[0], tdTextures_[1], tdTextures_[2], tdTextures_[3] });

        tdDepth_ = graphics::CreateRenderBuffer(Window::getWindowSize().x, Window::getWindowSize().y);
        graphics::AttachRenderBufferToFBO(tdDepth_);
        graphics::FinalizeFBO();
        graphics::BindFrameBuffer();

        tdLightingShader_ = new Shader;
        tdLightingShader_->AttachShaderFromFile(GL_COMPUTE_SHADER, PG_RESOURCE_DIR "shaders/compute.glsl");
        tdLightingShader_->CreateAndLinkProgram();
        tdLightingShader_->AutoDetectVariables();

        glGenTextures(1, &tdLightingOutput_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tdLightingOutput_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, Window::getWindowSize().x, Window::getWindowSize().y, 0, GL_RGBA, GL_FLOAT, NULL);
        glBindImageTexture(0, tdLightingOutput_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

        glGenBuffers(1, &lightSSBO_);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO_);
        glBufferData(GL_SHADER_STORAGE_BUFFER, 2 * sizeof(glm::vec4) * maxNumLights_, NULL, GL_STREAM_COPY);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, lightSSBO_);
    }

    // TODO: Actually release the openGL stuff!
    void RenderSystem::Free() {
        for (auto& subsys : subSystems_)
            delete subsys.second;
    }

    void RenderSystem::Render(Scene* scene, Camera* camera) {
        if (!camera) {
            camera = scene->GetCamera(0);
        }

        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        graphics::ToggleDepthTesting(true);
        // graphics::ToggleCulling(true);


        if (camera->GetRenderingPipeline() == RenderingPipeline::TILED_DEFERRED) {
            graphics::BindFrameBuffer(tdGbuffer_);
            graphics::SetClearColor(glm::vec4(0));
            graphics::Clear();
        }

        UpdateLights(scene, camera);

        for (const auto& subsys : subSystems_)
            subsys.second->Render(scene, *camera);

        if (camera->GetRenderingPipeline() == RenderingPipeline::TILED_DEFERRED) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            graphics::Clear();

            //// blit the deferred depth buffer to the main screen
            glBindFramebuffer(GL_READ_FRAMEBUFFER, tdGbuffer_);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBlitFramebuffer(0, 0, Window::getWindowSize().x, Window::getWindowSize().y, 0, 0, Window::getWindowSize().x, Window::getWindowSize().y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            auto& computeShader = *tdLightingShader_;
            computeShader.Enable();
            glBindImageTexture(0, tdLightingOutput_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
            glBindImageTexture(1, tdTextures_[0], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
            glBindImageTexture(2, tdTextures_[1], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
            glBindImageTexture(3, tdTextures_[2], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
            glBindImageTexture(4, tdTextures_[3], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);

            glUniform2i(computeShader["screenSize"], Window::getWindowSize().x, Window::getWindowSize().y);
            glUniform1i(computeShader["numPointLights"], numPointLights_);
            glUniform1i(computeShader["numDirectionalLights"], numDirectionalLights_);
            glUniformMatrix4fv(computeShader["invProjMatrix"], 1, GL_FALSE, glm::value_ptr(glm::inverse(camera->GetP())));

            const int BLOCK_SIZE = 16;
            glDispatchCompute(Window::getWindowSize().x / BLOCK_SIZE, Window::getWindowSize().y / BLOCK_SIZE, 1);

            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

            graphics::ToggleDepthBufferWriting(false);
            graphics::ToggleDepthTesting(false);

            tdCombineShader_->Enable();
            glBindVertexArray(tdQuadVAO_);
            graphics::Bind2DTexture(tdLightingOutput_, (*tdCombineShader_)["computeOutput"], 0);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            graphics::ToggleDepthTesting(true);
            graphics::ToggleDepthBufferWriting(true);
        }
    }

    void RenderSystem::UpdateLights(Scene* scene, Camera* camera) {
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

        for (i = 0; i < pointLights.size() && (i + dirLights.size()) < maxNumLights_; ++i) {
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
