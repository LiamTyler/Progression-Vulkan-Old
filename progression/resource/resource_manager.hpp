#pragma once

#include "core/config.hpp"
#include "graphics/model.hpp"
#include "graphics/material.hpp"
#include "graphics/shader.hpp"
#include "graphics/skybox.hpp"
#include "graphics/texture2D.hpp"
#include <unordered_map>
#include <memory>

namespace Progression {

    namespace {

        std::unordered_map<std::string, Model> models_;
        std::unordered_map<std::string, Material> materials_;
        std::unordered_map<std::string, Texture2D> textures2D_;
        std::unordered_map<std::string, Shader> shaders_;
        std::unordered_map<std::string, Skybox> skyboxes_;
        std::unordered_map<std::shared_ptr<Model>, std::vector<std::shared_ptr<Material>>> modelToLoadedMaterials_;

    } // namespace anonymous

    namespace ResourceManager {

        void init(const config::Config& config);
        void shutdown();

        bool roadResourceFile(const std::string& relativePath);

    };

} // namespace Progression
