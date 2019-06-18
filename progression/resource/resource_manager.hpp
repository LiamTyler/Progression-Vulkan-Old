#pragma once

#include "resource/model.hpp"
#include "resource/material.hpp"
#include "resource/shader.hpp"
#include "resource/resourceIO/shader_io.hpp"
#include "resource/texture2D.hpp"
// #include "resource/skybox.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>




namespace Progression {

    namespace Resource {

        void init();
        void update();
        void join();
        void shutdown();

        Model*      getModel(const std::string& name);
        Material*   getMaterial(const std::string& name);
        Texture2D*  getTexture2D(const std::string& name);
        Shader*     getShader(const std::string& name);

        Model*                 loadModel(const std::string& name, const std::string& fname,
                                         bool optimize = true, bool freeCPUCopy = true);
        std::vector<Material*> loadMaterials(const std::string& fname);
        Texture2D*             loadTexture2D(const std::string& name, const std::string& fname,
                                             const TextureUsageDesc& desc, bool freeCPUCopy = true);
        Shader*                loadShader(const std::string& name, const ShaderFileDesc& desc);

        // for all the add functions, the resource will be moved into the manager, meaning that
        // any other reference to it will be invalidated. Get the newly managed copy with the get
        // functions
        void addModel(const std::string& name, Model* model);
        void addMaterial(const std::string& name, Material* mat);
        void addTexture2D(const std::string& name, Texture2D* tex);
        void addShader(const std::string& name, Shader* shader);

        bool loadResourceFile(const std::string& fname);

    } // namespace Resource

} // namespace Progression
