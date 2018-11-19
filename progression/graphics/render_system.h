#pragma once

#include "core/config.h"
#include "graphics/shader.h"
#include "core/camera.h"
#include "graphics/material.h"

#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace Progression {

	enum RenderOptions : uint64_t {
		BLOOM = 0x1,
	};

    class Scene;
    class RenderSubSystem;

    class RenderSystem {

		struct PostProcessing {
			GLuint FBO;
			GLuint mainBuffer;
			GLuint glowBuffer;
			GLuint depthBuffer;
			GLuint subGlowBuffers[5][2];
			float* bloomKernels[5];
			unsigned int bloomKernelSizes[5];
			int bloomLevels;
			GLuint pingPongFBO;
			GLuint glowBlurredBuffer;
			float exposure;
			float bloomIntensity;
			Shader* copyShader;
			Shader* blurShader;
			Shader* bloomCombineShader;
		};

    public:
        RenderSystem() = delete;
        virtual ~RenderSystem() = delete;

        static void Init(const config::Config& config);
        static void Free();

        static void Render(Scene* scene, Camera* camera = nullptr);

        template<typename T>
        static T* GetSubSystem() {
            if (subSystems_.find(typeid(T)) == subSystems_.end())
                return nullptr;
            else
                return (T*) subSystems_[typeid(T)];
        }

		static void EnableOption(uint64_t option);
		static void DisableOption(uint64_t option);
		static bool GetOption(uint64_t option);

        static void UpdateLights(Scene* scene, Camera* camera);
        static void UploadLights(Shader& shader);
        static void UploadCameraProjection(Shader& shader, Camera& camera);
        static void UploadMaterial(Shader& shader, Material& material);

    private:
        static std::unordered_map<std::type_index, RenderSubSystem*> subSystems_;
        static unsigned int numDirectionalLights_;
        static unsigned int numPointLights_;
        static unsigned int maxNumLights_;
        static GLuint lightSSBO_;
        static float lightIntensityCutoff_;
        static GLuint tdGbuffer_;
        static GLuint tdTextures_[6];
        static GLuint tdDepth_;
        static Shader* tdLightingShader_;
        static GLuint tdLightingOutput_;
        static Shader* tdCombineShader_;
        static GLuint quadVAO_;
        static GLuint quadVBO_;
		static struct PostProcessing postProcessingData_;
		static uint64_t options_;
    };

} // namespace Progression
