#include "graphics/render_system.hpp"
#include "graphics/render_subsystem.hpp"
#include "graphics/graphics_api.hpp"
#include "core/scene.hpp"
#include "core/camera.hpp"
#include "graphics/mesh_render_subsystem.hpp"
#include "core/window.hpp"
#include "utils/logger.hpp"

namespace Progression {

    std::unordered_map<std::type_index, RenderSubSystem*> RenderSystem::subSystems_ = {};
    GLuint RenderSystem::quadVAO_ = 0;
    GLuint RenderSystem::quadVBO_ = 0;
    GLuint RenderSystem::cubeVAO_ = 0;
    GLuint RenderSystem::cubeVBO_ = 0;
    Shader RenderSystem::backgroundShader_ = {};
    uint64_t RenderSystem::options_ = 0;

    unsigned int RenderSystem::numDirectionalLights_ = 0;
    unsigned int RenderSystem::numPointLights_ = 0;
    GLuint RenderSystem::lightSSBO_ = 0;
    unsigned int RenderSystem::maxNumLights_ = 0;
    float RenderSystem::lightIntensityCutoff_ = 0;
    glm::vec4* RenderSystem::cpuLightBuffer_ = nullptr;

    bool RenderSystem::tdEnabled_ = true;
    GLuint RenderSystem::tdGbuffer_ = 0;
    GLuint RenderSystem::tdGBufferTextures_[6] = {0, 0, 0, 0, 0, 0};
    Shader RenderSystem::tdComputeShader_ = {};

    RenderSystem::PostProcessing RenderSystem::postProcess_;
    glm::vec3 RenderSystem::ambientLight = glm::vec3(.1);

    void RenderSystem::Init(const config::Config& config) {
        // auto load the mesh renderer subsystem
        subSystems_[typeid(MeshRenderSubSystem)] = new MeshRenderSubSystem;

        auto rsConfig = config->get_table("renderSystem");
        if (!rsConfig) {
            LOG_ERR("Need to specify the 'renderSystem' in the config file");
            exit(EXIT_FAILURE);
        }

        // parse the config file options
        maxNumLights_         = rsConfig->get_as<int>("maxNumLights").value_or(10001);
        lightIntensityCutoff_ = rsConfig->get_as<float>("lightIntensityCutoff").value_or(0.03f);
        tdEnabled_            = rsConfig->get_as<bool>("enableTiledDeferredPipeline").value_or(true);

        // setup the lighting data
        cpuLightBuffer_ = new glm::vec4[2 * maxNumLights_];
        glGenBuffers(1, &lightSSBO_);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO_);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, lightSSBO_);
        glBufferData(GL_SHADER_STORAGE_BUFFER, 2 * sizeof(glm::vec4) * maxNumLights_, NULL, GL_DYNAMIC_COPY);

        // setup the general data stuff
        float quadVerts[] = {
            -1, 1,
            -1, -1,
            1, -1,

            -1, 1,
            1, -1,
            1, 1
        };
        glGenVertexArrays(1, &quadVAO_);
        glBindVertexArray(quadVAO_);
        glGenBuffers(1, &quadVBO_);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

        float skyboxVertices[] = {
            // positions          
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f
        };

        glGenVertexArrays(1, &cubeVAO_);
        glBindVertexArray(cubeVAO_);
        glGenBuffers(1, &cubeVBO_);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        if (!backgroundShader_.Load(PG_RESOURCE_DIR "shaders/background.vert", PG_RESOURCE_DIR "shaders/background.frag")) {
            LOG_ERR("Could not load the background shader");
            exit(EXIT_FAILURE);
        }

        // setup the tiled deferred stuff
        if (tdEnabled_) {
            tdGbuffer_ = graphics::CreateFrameBuffer();
            // [0] = pos, [1] = normals, [2] = diffuse, [3] = spec+exp, [4] = emissive, [5] = depth
            tdGBufferTextures_[0] = graphics::Create2DTexture(Window::width(), Window::height(), GL_RGBA32F);
            tdGBufferTextures_[1] = graphics::Create2DTexture(Window::width(), Window::height(), GL_RGBA32F);
            tdGBufferTextures_[2] = graphics::Create2DTexture(Window::width(), Window::height(), GL_RGBA);
            tdGBufferTextures_[3] = graphics::Create2DTexture(Window::width(), Window::height(), GL_RGBA32F);
            tdGBufferTextures_[4] = graphics::Create2DTexture(Window::width(), Window::height(), GL_RGBA16F);
            graphics::AttachColorTexturesToFBO({ tdGBufferTextures_[0], tdGBufferTextures_[1], tdGBufferTextures_[2], tdGBufferTextures_[3], tdGBufferTextures_[4] });

            tdGBufferTextures_[5] = graphics::CreateRenderBuffer(Window::width(), Window::height());
            graphics::AttachRenderBufferToFBO(tdGBufferTextures_[5]);
            graphics::FinalizeFBO();
            graphics::BindFrameBuffer();

            if (!tdComputeShader_.Load(PG_RESOURCE_DIR "shaders/tiled_deferred_compute.glsl")) {
                LOG_ERR("Tiled deferred pipeline enabled, but cant load the shaders. Exiting");
                exit(EXIT_FAILURE);
            }
        }

        // setup the post processing stuff
        if (!postProcess_.shader.Load(PG_RESOURCE_DIR "shaders/post_process.vert", PG_RESOURCE_DIR "shaders/post_process.frag")) {
            LOG_ERR("Cant load the post processing shader. Exiting");
            exit(EXIT_FAILURE);
        }
        postProcess_.FBO = graphics::CreateFrameBuffer();
        postProcess_.colorTex = graphics::Create2DTexture(Window::width(), Window::height(), GL_RGBA16F, GL_LINEAR, GL_LINEAR);
        graphics::AttachColorTexturesToFBO({ postProcess_.colorTex });
        postProcess_.depthTex = graphics::CreateRenderBuffer(Window::width(), Window::height());
        graphics::AttachRenderBufferToFBO(postProcess_.depthTex);
        graphics::FinalizeFBO();

        postProcess_.exposure = 1;
    }

    void RenderSystem::Free() {
        for (auto& subsys : subSystems_)
            delete subsys.second;
        delete[] cpuLightBuffer_;
        glDeleteVertexArrays(1, &quadVAO_);
        glDeleteBuffers(1, &quadVBO_);
        glDeleteVertexArrays(1, &cubeVAO_);
        glDeleteBuffers(1, &cubeVBO_);
        backgroundShader_.Free();
        glDeleteBuffers(1, &lightSSBO_);
        if (tdEnabled_) {
            glDeleteFramebuffers(1, &tdGbuffer_);
            tdComputeShader_.Free();
            if (tdGBufferTextures_[0] != -1) {
                glDeleteTextures(5, tdGBufferTextures_);
                glDeleteRenderbuffers(1, &tdGBufferTextures_[5]);
                for (int i = 0; i < 6; ++i)
                    tdGBufferTextures_[i] = -1;
            }
        }
        glDeleteFramebuffers(1, &postProcess_.FBO);
        glDeleteTextures(1, &postProcess_.colorTex);
        glDeleteRenderbuffers(1, &postProcess_.depthTex);
        postProcess_.shader.Free();
    }

    void RenderSystem::Render(Scene* scene, Camera* camera) {
        if (!camera) {
            camera = scene->GetCamera(0);
        }

        graphics::ToggleDepthTesting(true);
        //graphics::ToggleCulling(true);

        glBindFramebuffer(GL_FRAMEBUFFER, postProcess_.FBO);
        graphics::SetClearColor(glm::vec4(0));
        graphics::Clear();

        if (camera->GetRenderingPipeline() == RenderingPipeline::TILED_DEFERRED) {
            graphics::BindFrameBuffer(tdGbuffer_);
            graphics::SetClearColor(glm::vec4(0));
            graphics::Clear();
        }

        UpdateLights(scene, camera);

        for (const auto& subsys : subSystems_)
            subsys.second->Render(scene, *camera);

        if (camera->GetRenderingPipeline() == RenderingPipeline::TILED_DEFERRED) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, tdGbuffer_);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, postProcess_.FBO);
            glBlitFramebuffer(0, 0, Window::width(), Window::height(), 0, 0, Window::width(), Window::height(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_FRAMEBUFFER, postProcess_.FBO);

            tdComputeShader_.Enable();
            glBindImageTexture(0, postProcess_.colorTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
            glBindImageTexture(2, tdGBufferTextures_[0], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
            glBindImageTexture(3, tdGBufferTextures_[1], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
            glBindImageTexture(4, tdGBufferTextures_[2], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
            glBindImageTexture(5, tdGBufferTextures_[3], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
            glBindImageTexture(6, tdGBufferTextures_[4], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);

            glUniform2i(tdComputeShader_["screenSize"], Window::width(), Window::height());
            glUniform1i(tdComputeShader_["numPointLights"], numPointLights_);
            glUniform1i(tdComputeShader_["numDirectionalLights"], numDirectionalLights_);
            glUniformMatrix4fv(tdComputeShader_["invProjMatrix"], 1, GL_FALSE, glm::value_ptr(glm::inverse(camera->GetP())));

            const int BLOCK_SIZE = 16;
            glDispatchCompute(Window::width() / BLOCK_SIZE, Window::height() / BLOCK_SIZE, 1);

            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }

        // Bind and clear the main screen
        graphics::BindFrameBuffer(0);
        glViewport(0, 0, Window::width(), Window::height());
        graphics::SetClearColor(glm::vec4(0));
        graphics::Clear();

        // Copy the post processing depth buffer to the main screen
        graphics::ToggleDepthBufferWriting(false);
        graphics::ToggleDepthTesting(false);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, postProcess_.FBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, Window::width(), Window::height(), 0, 0, Window::width(), Window::height(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        // Draw the post processing color texture to screen, while performing
        // post processing effects, tone mapping, and gamma correction
        postProcess_.shader.Enable();
        glUniform1f(postProcess_.shader["exposure"], postProcess_.exposure);
        graphics::Bind2DTexture(postProcess_.colorTex, postProcess_.shader["originalColor"], 0);
        glBindVertexArray(quadVAO_);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        graphics::ToggleDepthTesting(true);
        graphics::ToggleDepthBufferWriting(true);

        if (scene->getSkybox()) {
            RenderSkybox(*scene->getSkybox(), *camera);
        } else {
            backgroundShader_.Enable();
            glUniformMatrix4fv(backgroundShader_["MVP"], 1, GL_FALSE, glm::value_ptr(glm::mat4(1)));
            glUniform1i(backgroundShader_["skybox"], false);
            glUniform3fv(backgroundShader_["color"], 1, glm::value_ptr(scene->GetBackgroundColor()));
            glDepthMask(GL_FALSE);
            glBindVertexArray(quadVAO_);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glDepthMask(GL_TRUE);
        }
    }

    void RenderSystem::EnableOption(uint64_t option) {
        options_ |= option;
    }

    void RenderSystem::DisableOption(uint64_t option) {
        options_ &= (~option);
    }

    bool RenderSystem::GetOption(uint64_t option) {
        return options_ & option;
    }

    void RenderSystem::UpdateLights(Scene* scene, Camera* camera) {
        const auto& dirLights = scene->GetDirectionalLights();
        const auto& pointLights = scene->GetPointLights();
        numDirectionalLights_ = dirLights.size();
        numPointLights_ = pointLights.size();
        glm::mat4 V = camera->GetV();

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO_);
        for (int i = 0; i < dirLights.size(); ++i) {
            glm::vec3 dir(0, 0, -1);
            glm::mat4 rot(1);
            rot = glm::rotate(rot, dirLights[i]->transform.rotation.z, glm::vec3(0, 0, 1));
            rot = glm::rotate(rot, dirLights[i]->transform.rotation.y, glm::vec3(0, 1, 0));
            rot = glm::rotate(rot, dirLights[i]->transform.rotation.x, glm::vec3(1, 0, 0));
            cpuLightBuffer_[2 * i + 0] = V * rot * glm::vec4(dir, 0);
            cpuLightBuffer_[2 * i + 1] = glm::vec4(dirLights[i]->intensity * dirLights[i]->color, 1);
        }

        for (int i = 0; i < pointLights.size(); ++i) {
            const auto& pl = pointLights[i];
            cpuLightBuffer_[2 * (numDirectionalLights_ + i) + 0] = V * glm::vec4(pl->transform.position, 1);
            cpuLightBuffer_[2 * (numDirectionalLights_ + i) + 0].w = pl->radius * pl->radius;
            cpuLightBuffer_[2 * (numDirectionalLights_ + i) + 1] = glm::vec4(pl->intensity * pl->color, 1);
        }
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 2 * (numDirectionalLights_ + numPointLights_) * sizeof(glm::vec4), cpuLightBuffer_);
    }


    void RenderSystem::UploadLights(Shader& shader) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO_);
        glUniform1i(shader["numDirectionalLights"], numDirectionalLights_);
        glUniform1i(shader["numPointLights"], numPointLights_);
        glUniform3fv(shader["ambientLight"], 1, glm::value_ptr(ambientLight));
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

    void RenderSystem::RenderSkybox(const Skybox& skybox, const Camera& camera) {
        backgroundShader_.Enable();
        glm::mat4 P = camera.GetP();
        glm::mat4 RV = glm::mat4(glm::mat3(camera.GetV()));
        glUniformMatrix4fv(backgroundShader_["MVP"], 1, GL_FALSE, glm::value_ptr(P * RV));
        glUniform1i(backgroundShader_["skybox"], true);
        glDepthMask(GL_FALSE);
        glBindVertexArray(cubeVAO_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.getGPUHandle());
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthMask(GL_TRUE);
    }

} // namespace Progression
