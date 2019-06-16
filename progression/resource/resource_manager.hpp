#pragma once

#include "resource/model.hpp"
#include "resource/material.hpp"
#include "resource/shader.hpp"
#include "resource/resourceIO/shader_io.hpp"
#include "resource/texture2D.hpp"
// #include "resource/skybox.hpp"
#include <unordered_map>
#include <memory>

namespace Progression {

    namespace {

        std::unordered_map<std::string, Model> models_;
        std::unordered_map<std::string, Material> materials_;
        std::unordered_map<std::string, Texture2D> textures2D_;
        std::unordered_map<std::string, Shader> shaders_;
        // std::unordered_map<std::string, Skybox> skyboxes_;
        std::unordered_map<std::shared_ptr<Model>, std::vector<std::shared_ptr<Material>>> modelToLoadedMaterials_;

    } // namespace anonymous

    namespace Resource{

        void init();
        void shutdown();

        Model*      getModel(const std::string& name);
        Material*   getMaterial(const std::string& name);
        Texture2D*  getTexture2D(const std::string& name);
        Shader*     getShader(const std::string& name);

        Model*                 loadModel(const std::string& fname, bool optimize = true, bool freeCPUCopy = true);
        std::vector<Material*> loadMaterials(const std::string& fname);
        Texture2D*             loadTexture2D(const std::string& fname, const TextureUsageDesc& desc);
        Shader*                loadShader(const std::string& name, const ShaderFilesDesc& desc);
        // Skybox*     loadSkybox(const std::string& name);

        bool loadResourceFile(const std::string& fname);

    } // namespace Resource

} // namespace Progression
