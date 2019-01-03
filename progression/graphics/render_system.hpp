#pragma once

#include "core/config.hpp"
#include "graphics/shader.hpp"
#include "core/camera.hpp"
#include "graphics/material.hpp"

#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace Progression {

	enum RenderOptions : uint64_t {
		// BLOOM = 0x1,
	};

    class Scene;
    class RenderSubSystem;

    class RenderSystem {

		struct PostProcessing {
			GLuint FBO;
			GLuint mainBuffer;
			GLuint depthBuffer;
			float exposure;
            Shader* shader;
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
        // general data
        static std::unordered_map<std::type_index, RenderSubSystem*> subSystems_;
        static GLuint quadVAO_;
        static GLuint quadVBO_;
        static uint64_t options_;
        static Shader* drawTexShader_;

        // lighting data
        static unsigned int numDirectionalLights_;
        static unsigned int numPointLights_;
        static unsigned int maxNumLights_;
        static GLuint lightSSBO_;
        static glm::vec4* cpuLightBuffer_;
        static float lightIntensityCutoff_;

        // tiled deferred data
        static GLuint tdGbuffer_;
        static GLuint tdGBufferTextures_[6]; // 5 color, 1 depth
        static Shader* tdLightingShader_;
        static GLuint tdLightingOutput_;

        // post process data
        static struct PostProcessing postProcess_;
    };

} // namespace Progression
