#include "graphics/render_system.h"
#include "graphics/render_subsystem.h"
#include "graphics/graphics_api.h"
#include "core/scene.h"
#include "core/camera.h"
#include "graphics/mesh_render_subsystem.h"
#include "core/window.h"

static float* genKernel(int size, float sigma) {
	float* kernel = new float[size];
	int halfSize = size / 2.0f;
	float sum = 0;
	for (int i = 0; i < size; i++) {
		kernel[i] = 1.0f / (sqrtf(2 * M_PI) * sigma)  * exp(-0.5 * pow((i - halfSize) / sigma, 2.0));
		sum += kernel[i];
	}
	for (int i = 0; i < size; i++) {
		kernel[i] /= sum;
		std::cout << std::setprecision(4) << kernel[i] << " ";
	}
	std::cout << std::endl << std::endl;

	return kernel;
}

namespace Progression {

    std::unordered_map<std::type_index, RenderSubSystem*> RenderSystem::subSystems_ = {};
    unsigned int RenderSystem::numDirectionalLights_ = 0;
    unsigned int RenderSystem::numPointLights_ = 0;
    GLuint RenderSystem::lightSSBO_ = 0;
    unsigned int RenderSystem::maxNumLights_ = 0;
    float RenderSystem::lightIntensityCutoff_ = 0;
    GLuint RenderSystem::tdGbuffer_ = 0;
    GLuint RenderSystem::tdTextures_[6] = {0, 0, 0, 0, 0, 0};
	GLuint RenderSystem::tdLightingOutput_ = 0;
	GLuint RenderSystem::tdDepth_ = 0;
	Shader* RenderSystem::tdLightingShader_ = nullptr;
    Shader* RenderSystem::tdCombineShader_ = nullptr;
    GLuint RenderSystem::quadVAO_ = 0;
    GLuint RenderSystem::quadVBO_ = 0;
	RenderSystem::PostProcessing RenderSystem::postProcessingData_;
	uint64_t RenderSystem::options_ = 0;

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

		tdCombineShader_ = new Shader(PG_RESOURCE_DIR "shaders/deferred_combine.vert", PG_RESOURCE_DIR "shaders/deferred_combine.frag");

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
		glEnableVertexAttribArray((*tdCombineShader_)["vertex"]);
		glVertexAttribPointer((*tdCombineShader_)["vertex"], 2, GL_FLOAT, GL_FALSE, 0, 0);

		tdGbuffer_ = graphics::CreateFrameBuffer();
		tdTextures_[2] = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA32F);
		tdTextures_[3] = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA32F);
		tdTextures_[4] = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA);
		tdTextures_[5] = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA32F);
		tdTextures_[1] = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA16F);

		// graphics::AttachColorTexturesToFBO({ tdTextures_[2], tdTextures_[3], tdTextures_[4], tdTextures_[5] });
		graphics::AttachColorTexturesToFBO({ tdTextures_[2], tdTextures_[3], tdTextures_[4], tdTextures_[5], tdTextures_[1] });

		tdDepth_ = graphics::CreateRenderBuffer(Window::getWindowSize().x, Window::getWindowSize().y);
		graphics::AttachRenderBufferToFBO(tdDepth_);
		graphics::FinalizeFBO();
		graphics::BindFrameBuffer();

		tdLightingShader_ = new Shader;
		tdLightingShader_->AttachShaderFromFile(GL_COMPUTE_SHADER, PG_RESOURCE_DIR "shaders/compute.glsl");
		tdLightingShader_->CreateAndLinkProgram();
		tdLightingShader_->AutoDetectVariables();
		
		/*
		glGenTextures(1, &tdLightingOutput_);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tdLightingOutput_);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, Window::getWindowSize().x, Window::getWindowSize().y, 0, GL_RGBA, GL_FLOAT, NULL);
		glBindImageTexture(0, tdLightingOutput_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		*/
		
		//tdTextures_[0] = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA32F, GL_LINEAR, GL_LINEAR);
		//tdTextures_[1] = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA32F, GL_LINEAR, GL_LINEAR);
		//glBindImageTexture(0, tdTextures_[0], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		//glBindImageTexture(1, tdTextures_[1], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glBindImageTexture(0, postProcessingData_.mainBuffer, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
		glBindImageTexture(1, postProcessingData_.glowBuffer, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);

		glGenBuffers(1, &lightSSBO_);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO_);
		glBufferData(GL_SHADER_STORAGE_BUFFER, 2 * sizeof(glm::vec4) * maxNumLights_, NULL, GL_STREAM_COPY);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, lightSSBO_);

		postProcessingData_.bloomCombineShader = new Shader(PG_RESOURCE_DIR "shaders/bloomCombine.vert", PG_RESOURCE_DIR "shaders/bloomCombine.frag");
		postProcessingData_.blurShader = new Shader(PG_RESOURCE_DIR "shaders/blur.vert", PG_RESOURCE_DIR "shaders/blur.frag");
		postProcessingData_.blurShader->AddUniform("kernel");
		postProcessingData_.copyShader = new Shader(PG_RESOURCE_DIR "shaders/copy.vert", PG_RESOURCE_DIR "shaders/copy.frag");

		postProcessingData_.FBO = graphics::CreateFrameBuffer();
		postProcessingData_.mainBuffer = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA16F, GL_LINEAR, GL_LINEAR);
		postProcessingData_.glowBuffer = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA16F, GL_LINEAR, GL_LINEAR);
		graphics::AttachColorTexturesToFBO({ postProcessingData_.mainBuffer, postProcessingData_.glowBuffer });
		postProcessingData_.depthBuffer = graphics::CreateRenderBuffer(Window::getWindowSize().x, Window::getWindowSize().y);
		graphics::AttachRenderBufferToFBO(postProcessingData_.depthBuffer);
		graphics::FinalizeFBO();

		postProcessingData_.pingPongFBO = graphics::CreateFrameBuffer();
		postProcessingData_.glowBlurredBuffer = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA16F, GL_LINEAR, GL_LINEAR);
		graphics::AttachColorTexturesToFBO({ postProcessingData_.glowBlurredBuffer });
		graphics::FinalizeFBO();

		postProcessingData_.bloomLevels = 4;
		postProcessingData_.exposure = 1;
		postProcessingData_.bloomIntensity = .3;

		int divisor = 2;
		for (int i = 0; i < 4; ++i) {
			postProcessingData_.subGlowBuffers[i][0] = graphics::Create2DTexture(Window::getWindowSize().x / divisor, Window::getWindowSize().y / divisor, GL_RGBA16F, GL_LINEAR, GL_LINEAR);
			postProcessingData_.subGlowBuffers[i][1] = graphics::Create2DTexture(Window::getWindowSize().x / divisor, Window::getWindowSize().y / divisor, GL_RGBA16F, GL_LINEAR, GL_LINEAR);
			divisor *= 2;
		}

		int kernels = 4;
		int sizes[] = { 5, 11, 21, 41 };
		int stddev[] = { 3, 3, 5, 11 };
		for (int i = 0; i < kernels; ++i) {
			postProcessingData_.bloomKernels[i] = genKernel(sizes[i], stddev[i]);
			postProcessingData_.bloomKernelSizes[i] = sizes[i];
		}
    }

    // TODO: Actually release the openGL stuff!
    void RenderSystem::Free() {
        for (auto& subsys : subSystems_)
            delete subsys.second;
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
			glBindImageTexture(1, postProcessingData_.glowBuffer, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
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

			/*
            graphics::ToggleDepthBufferWriting(false);
            graphics::ToggleDepthTesting(false);

            tdCombineShader_->Enable();
            glBindVertexArray(quadVAO_);
			graphics::Bind2DTexture(postProcessingData_.mainBuffer, (*tdCombineShader_)["computeOutput"], 0);
			//graphics::Bind2DTexture(tdLightingOutput_, (*tdCombineShader_)["computeOutput"], 0);
			glDrawArrays(GL_TRIANGLES, 0, 6);

            graphics::ToggleDepthTesting(true);
            graphics::ToggleDepthBufferWriting(true);
			*/
        }
		

		if (options_ & BLOOM) {
			graphics::BindFrameBuffer(postProcessingData_.pingPongFBO);
			postProcessingData_.copyShader->Enable();
			glBindVertexArray(quadVAO_);
			graphics::Bind2DTexture(postProcessingData_.glowBuffer, postProcessingData_.copyShader->GetUniform("tex"), 0);

			int divisor = 2;
			for (int i = 0; i < postProcessingData_.bloomLevels; ++i) {
				glViewport(0, 0, Window::getWindowSize().x / divisor, Window::getWindowSize().y / divisor);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postProcessingData_.subGlowBuffers[i][0], 0);
				glDrawArrays(GL_TRIANGLES, 0, 6);
				divisor *= 2;
			}

			postProcessingData_.blurShader->Enable();
			glBindVertexArray(quadVAO_);
			graphics::ToggleDepthBufferWriting(false);

			divisor = 2;
			for (int i = 0; i < postProcessingData_.bloomLevels; ++i) {
				glUniform1fv(postProcessingData_.blurShader->GetUniform("kernel"), postProcessingData_.bloomKernelSizes[i], postProcessingData_.bloomKernels[i]);
				glUniform1i(postProcessingData_.blurShader->GetUniform("halfKernelWidth"), postProcessingData_.bloomKernelSizes[i] / 2);

				glViewport(0, 0, Window::getWindowSize().x / divisor, Window::getWindowSize().y / divisor);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postProcessingData_.subGlowBuffers[i][1], 0);
				graphics::Bind2DTexture(postProcessingData_.subGlowBuffers[i][0], postProcessingData_.blurShader->GetUniform("tex"), 0);
				glUniform2f(postProcessingData_.blurShader->GetUniform("offset"), static_cast<float>(divisor) / Window::getWindowSize().x, 0);
				glDrawArrays(GL_TRIANGLES, 0, 6);

				graphics::Bind2DTexture(postProcessingData_.subGlowBuffers[i][1], postProcessingData_.blurShader->GetUniform("tex"), 0);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, postProcessingData_.subGlowBuffers[i][0], 0);
				glUniform2f(postProcessingData_.blurShader->GetUniform("offset"), 0, static_cast<float>(divisor) / Window::getWindowSize().y);
				glDrawArrays(GL_TRIANGLES, 0, 6);
				divisor *= 2;
			}

			graphics::ToggleDepthBufferWriting(true);
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

		auto& bloomCombineShader = *postProcessingData_.bloomCombineShader;
		bloomCombineShader.Enable();
		glUniform1f(bloomCombineShader["bloomIntensity"], postProcessingData_.bloomIntensity);
		glUniform1f(bloomCombineShader["exposure"], postProcessingData_.exposure);
		graphics::Bind2DTexture(postProcessingData_.mainBuffer, bloomCombineShader["originalColor"], 0);
		glBindVertexArray(quadVAO_);

		if (options_ & BLOOM) {
			glUniform1i(bloomCombineShader["bloom"], true);
			
			//glUniform1i(bloomCombineShader["bloomLevels"], postProcessingData_.bloomLevels);
			for (int i = 1; i <= postProcessingData_.bloomLevels; ++i) {
				std::string num = "blur" + std::to_string(i);
				graphics::Bind2DTexture(postProcessingData_.subGlowBuffers[i][0], bloomCombineShader[num], i);
			}
		} else {
			glUniform1i(bloomCombineShader["bloom"], false);
		}

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
        GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
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
