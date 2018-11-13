#pragma once

#include "core/config.h"
#include "graphics/shader.h"
#include "core/camera.h"
#include "graphics/material.h"

#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace Progression {

    class Scene;
    class RenderSubSystem;

    class RenderSystem {
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

        static void UpdateLights(Scene* scene, Camera* camera);
        static void UploadLights(Shader& shader);
        static void UploadCameraProjection(Shader& shader, Camera& camera);
        static void UploadMaterial(Shader& shader, Material& material);

    private:
        static std::unordered_map<std::type_index, RenderSubSystem*> subSystems_;
        static std::vector<glm::vec3> lightBuffer_;
        static unsigned int numDirectionalLights_;
        static unsigned int numPointLights_;
        static unsigned int maxNumLights_;
        static GLuint lightSSBO_;
        static float lightIntensityCutoff_;
        static GLuint tdGbuffer_;
        static GLuint tdTextures_[4];
        static GLuint tdDepth_;
        static Shader* tdLightingShader_;
        static GLuint tdLightingOutput_;
        static Shader* tdCombineShader_;
        static GLuint tdQuadVAO_;
        static GLuint tdQuadVBO_;
    };

} // namespace Progression
