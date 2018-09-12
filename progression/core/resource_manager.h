#pragma once

#include "core/config.h"
#include "core/game_object.h"
#include "graphics/model.h"
#include "graphics/material.h"
#include "graphics/shader.h"
#include "graphics/skybox.h"
#include <unordered_set>

namespace Progression {

    class ResourceManager {
    public:
        ResourceManager() = delete;
        ~ResourceManager() = delete;

        static void Init(const config::Config& config);
        static void Free();

        // TODO: implement loading all resources inside a folder into memory
        // static void LoadResouceFolder(const std::string& folder);

        static Model* LoadModel(const std::string& filename);
        
        static Model* GetModel(const std::string& name) {
            if (models_.find(name) != models_.end())
                return &models_[name];
            else
                return nullptr;
        }

        static Material* GetMaterial(const std::string& name) {
            if (materials_.find(name) != materials_.end())
                return &materials_[name];
            else
                return nullptr;
        }

        static Shader* GetShader(const std::string& name) {
            if (shaders_.find(name) != shaders_.end())
                return &shaders_[name];
            else
                return nullptr;
        }

        static Skybox* LoadSkybox(const std::vector<std::string>& textures);

    private:
        // full models containing meshes, their materials, and a the renderers
        static std::unordered_map<std::string, Model> models_;

        // materials
        static std::unordered_map<std::string, Material> materials_;

        // textures

        // shaders
        static std::unordered_map<std::string, Shader> shaders_;



        static std::string rootResourceDir_;
    };

} // namespace Progression