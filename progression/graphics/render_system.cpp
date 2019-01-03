#include "graphics/render_system.hpp"
#include "graphics/render_subsystem.hpp"
#include "graphics/graphics_api.hpp"
#include "core/scene.hpp"
#include "core/camera.hpp"
#include "graphics/mesh_render_subsystem.hpp"
#include "core/window.hpp"

namespace Progression {

    std::unordered_map<std::type_index, RenderSubSystem*> RenderSystem::subSystems_ = {};
    unsigned int RenderSystem::numDirectionalLights_ = 0;
    unsigned int RenderSystem::numPointLights_ = 0;
    GLuint RenderSystem::lightSSBO_ = 0;
    unsigned int RenderSystem::maxNumLights_ = 0;
    float RenderSystem::lightIntensityCutoff_ = 0;
    glm::vec4* RenderSystem::cpuLightBuffer_ = nullptr;

    GLuint RenderSystem::tdGbuffer_ = 0;
    GLuint RenderSystem::tdGBufferTextures_[6] = {0, 0, 0, 0, 0, 0};
	GLuint RenderSystem::tdLightingOutput_ = 0;
	Shader* RenderSystem::tdLightingShader_ = nullptr;

    GLuint RenderSystem::quadVAO_ = 0;
    GLuint RenderSystem::quadVBO_ = 0;
	RenderSystem::PostProcessing RenderSystem::postProcess_;
    Shader* RenderSystem::drawTexShader_ = nullptr;
	uint64_t RenderSystem::options_ = 0;

	void RenderSystem::Init(const config::Config& config) {
		// auto load the mesh renderer subsystem
		subSystems_[typeid(MeshRenderSubSystem)] = new MeshRenderSubSystem;

		auto rsConfig = config->get_table("renderSystem");
		if (!rsConfig) {
			std::cout << "Need to specify the 'renderSystem' in the config file!" << std::endl;
			exit(0);
		}

        // setup the lighting data
		maxNumLights_ = rsConfig->get_as<int>("maxNumLights").value_or(10001);
        cpuLightBuffer_ = new glm::vec4[2 * maxNumLights_];
		lightIntensityCutoff_ = rsConfig->get_as<float>("lightIntensityCutoff").value_or(0.03f);
        glGenBuffers(1, &lightSSBO_);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO_);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, lightSSBO_);
        glBufferData(GL_SHADER_STORAGE_BUFFER, 2 * sizeof(glm::vec4) * maxNumLights_, NULL, GL_DYNAMIC_COPY);

        // setup the general data stuff
        drawTexShader_ = new Shader(PG_RESOURCE_DIR "shaders/quad.vert", PG_RESOURCE_DIR "shaders/drawTexture.frag");
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
		glEnableVertexAttribArray((*drawTexShader_)["vertex"]);
		glVertexAttribPointer((*drawTexShader_)["vertex"], 2, GL_FLOAT, GL_FALSE, 0, 0);

        // setup the tiled deferred stuff

		tdGbuffer_ = graphics::CreateFrameBuffer();
        tdGBufferTextures_[0] = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA32F);
        tdGBufferTextures_[1] = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA32F);
        tdGBufferTextures_[2] = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA);
        tdGBufferTextures_[3] = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA32F);
        tdGBufferTextures_[4] = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA16F);
		graphics::AttachColorTexturesToFBO({ tdGBufferTextures_[0], tdGBufferTextures_[1], tdGBufferTextures_[2], tdGBufferTextures_[3], tdGBufferTextures_[4] });

        tdGBufferTextures_[5] = graphics::CreateRenderBuffer(Window::getWindowSize().x, Window::getWindowSize().y);
		graphics::AttachRenderBufferToFBO(tdGBufferTextures_[5]);
		graphics::FinalizeFBO();
		graphics::BindFrameBuffer();

		tdLightingShader_ = new Shader;
		tdLightingShader_->AttachShaderFromFile(GL_COMPUTE_SHADER, PG_RESOURCE_DIR "shaders/compute.glsl");
		tdLightingShader_->CreateAndLinkProgram();
		tdLightingShader_->AutoDetectVariables();

		glBindImageTexture(0, postProcess_.mainBuffer, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

        // setup the post processing stuff
        postProcess_.FBO = graphics::CreateFrameBuffer();
        postProcess_.mainBuffer = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA16F, GL_LINEAR, GL_LINEAR);
        postProcess_.depthBuffer = graphics::CreateRenderBuffer(Window::getWindowSize().x, Window::getWindowSize().y);
		graphics::AttachRenderBufferToFBO(postProcess_.depthBuffer);
		graphics::FinalizeFBO();

		graphics::FinalizeFBO();

        postProcess_.exposure = 1;
    }

    // TODO: Actually release the openGL stuff!
    void RenderSystem::Free() {
        for (auto& subsys : subSystems_)
            delete subsys.second;
        if (cpuLightBuffer_)
            delete[] cpuLightBuffer_;
    }

	// TODO: Give a warning message if there is no camera, and return
    void RenderSystem::Render(Scene* scene, Camera* camera) {
        if (!camera) {
            camera = scene->GetCamera(0);
        }

		graphics::BindFrameBuffer();
		graphics::SetClearColor(glm::vec4(0));
		graphics::Clear();
        graphics::ToggleDepthTesting(true);
        //graphics::ToggleCulling(true);

		
		glBindFramebuffer(GL_FRAMEBUFFER, postProcessingData_.FBO);
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
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, postProcessingData_.FBO);
            glBlitFramebuffer(0, 0, Window::getWindowSize().x, Window::getWindowSize().y, 0, 0, Window::getWindowSize().x, Window::getWindowSize().y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            glBindFramebuffer(GL_FRAMEBUFFER, postProcessingData_.FBO);

            auto& computeShader = *tdLightingShader_;
            computeShader.Enable();
			glBindImageTexture(0, postProcessingData_.mainBuffer, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
			//glBindImageTexture(1, tdLightingOutput_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
			glBindImageTexture(2, tdTextures_[2], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
            glBindImageTexture(3, tdTextures_[3], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
            glBindImageTexture(4, tdTextures_[4], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
			glBindImageTexture(5, tdTextures_[5], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
			glBindImageTexture(6, tdTextures_[1], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);

            glUniform2i(computeShader["screenSize"], Window::getWindowSize().x, Window::getWindowSize().y);
            glUniform1i(computeShader["numPointLights"], numPointLights_);
            glUniform1i(computeShader["numDirectionalLights"], numDirectionalLights_);
            glUniformMatrix4fv(computeShader["invProjMatrix"], 1, GL_FALSE, glm::value_ptr(glm::inverse(camera->GetP())));

            const int BLOCK_SIZE = 16;
            glDispatchCompute(Window::getWindowSize().x / BLOCK_SIZE, Window::getWindowSize().y / BLOCK_SIZE, 1);

            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }
		
		graphics::BindFrameBuffer(0);
		glViewport(0, 0, Window::getWindowSize().x, Window::getWindowSize().y);

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		graphics::ToggleDepthBufferWriting(false);
		graphics::ToggleDepthTesting(false);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, postProcessingData_.FBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, Window::getWindowSize().x, Window::getWindowSize().y, 0, 0, Window::getWindowSize().x, Window::getWindowSize().y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

		glUniform1f(bloomCombineShader["exposure"], postProcessingData_.exposure);
		graphics::Bind2DTexture(postProcessingData_.mainBuffer, bloomCombineShader["originalColor"], 0);
		glBindVertexArray(quadVAO_);


		glDrawArrays(GL_TRIANGLES, 0, 6);

		graphics::ToggleDepthTesting(true);
		graphics::ToggleDepthBufferWriting(true);
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
			globalLights[2 * i + 0] = V * rot * glm::vec4(dir, 0);
			globalLights[2 * i + 1] = glm::vec4(dirLights[i]->intensity * dirLights[i]->color, 1);
		}

		for (int i = 0; i < pointLights.size(); ++i) {
			float lightRadius = sqrtf(pointLights[i]->intensity / lightIntensityCutoff_);
			globalLights[2 * (numDirectionalLights_ + i) + 0] = V * glm::vec4(pointLights[i]->transform.position, 1);
			globalLights[2 * (numDirectionalLights_ + i) + 0].w = lightRadius;
			globalLights[2 * (numDirectionalLights_ + i) + 1] = glm::vec4(pointLights[i]->intensity * pointLights[i]->color, 1);
		}
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 2 * (numDirectionalLights_ + numPointLights_) * sizeof(glm::vec4), globalLights);
		
		/*
        numPointLights_ = 0;
        numDirectionalLights_ = 0;

        const auto& dirLights = scene->GetDirectionalLights();
        const auto& pointLights = scene->GetPointLights();
        glm::mat4 V = camera->GetV();

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO_);
		glm::vec4* lightBuffer = (glm::vec4*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);

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
		*/
	}


    void RenderSystem::UploadLights(Shader& shader) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO_);
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
