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

        static void UploadLights(Shader& shader);
        static void UploadCameraProjection(Shader& shader, Camera& camera);
        static void UploadMaterial(Shader& shader, Material& material);

    protected:
        static std::unordered_map<std::type_index, RenderSubSystem*> subSystems_;
        static std::vector<glm::vec3> lightBuffer_;
        static unsigned int numDirectionalLights_;
        static unsigned int numPointLights_;
    };

} // namespace Progression