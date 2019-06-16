#include "resource/resource_manager.hpp"
#include "resource/mesh.hpp"
#include "core/configuration.hpp"
#include "resource/resourceIO/io.hpp"
#include "utils/logger.hpp"

namespace Progression { namespace Resource {

    void init() {
        // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        materials_["default"] = Material();
    }

    void shutdown() {
        models_.clear();
        materials_.clear();
        textures2D_.clear();
        shaders_.clear();
    }

    Model* getModel(const std::string& name) {
        auto it = models_.find(name);
        return it == models_.end() ? nullptr : &it->second;
    }

    Material* getMaterial(const std::string& name) {
        auto it = materials_.find(name);
        return it == materials_.end() ? nullptr : &it->second;
    }

    Texture2D* getTexture2D(const std::string& name) {
        auto it = textures2D_.find(name);
        return it == textures2D_.end() ? nullptr : &it->second;
    }

    Shader* getShader(const std::string& name) {
        auto it = shaders_.find(name);
        return it == shaders_.end() ? nullptr : &it->second;
    }


    Model* loadModel(const std::string& name, const std::string& fname, bool optimize, bool freeCPUCopy) {
        if (getModel(name)) {
            LOG_WARN("Reloading model that is already loaded");
        }

        models_[name] = std::move(Model());
        if (!loadModelFromObj(models_[name], fname, optimize, freeCPUCopy)) {
            LOG("Could not load model file: ", fname);
            models_.erase(name);
            return nullptr;
        }
        return &models_[name];
    }

    std::vector<Material*> loadMaterials(const std::string& fname) {
        std::vector<std::pair<std::string, Material>> materials;
        if (!loadMtlFile(materials, fname, PG_RESOURCE_DIR)) {
            LOG("Could not load mtl file: ", fname);
            return {};
        }

        std::vector<Material*> ret;
        for (const auto& pair : materials) {
            if (materials_.find(pair.first) != materials_.end())
                LOG_WARN("Resource manager already contains material with name: ", pair.first);

            materials_[pair.first] = std::move(pair.second);
            ret.push_back(&materials_[pair.first]);
        }

        return ret;
    }

    Texture2D* loadTexture2D(const std::string& name, const std::string& fname, const TextureUsageDesc& desc, bool freeCPUCopy) {
        if (getTexture2D(name)) {
            LOG_WARN("Reloading texture that is already loaded");
        }

        if (!loadTexture2D(textures2D_[name], fname, desc, freeCPUCopy)) {
            LOG("Could not load texture file: ", fname);
            textures2D_.erase(name);
            return nullptr;
        }

        return &textures2D_[name];
    }

    Shader* loadShader(const std::string& name, const ShaderFileDesc& desc) {
        if (getShader(name)) {
            LOG_WARN("Reloading shader that is already loaded");
        }

        if (!loadShaderFromText(shaders_[name], desc)) {
            LOG("Could not load shader ", name);
            shaders_.erase(name);
            return nullptr;
        }

        return &shaders_[name];
    }

    bool loadResourceFile(const std::string& fname) {
        std::ifstream in(fname);
        if (!in) {
            LOG_ERR("Could not open resource file:", fname);
            return false;
        }

        std::string line;
        while (std::getline(in, line)) {
            if (line == "")
                continue;
            if (line[0] == '#')
                continue;
            if (line == "Material") {
                if (!loadMaterialFromResourceFile(in)) {
                    LOG_ERR("could not parse and create material");
                    return false;
                }
            } else if (line == "Model") {
                if (!loadModelFromResourceFile(in)) {
                    LOG_ERR("could not parse and load model");
                    return false;
                }
            } else if (line == "Texture2D") {
                if (!loadTextureFromResourceFile(in)) {
                    LOG_ERR("Could not parse and load texture");
                    return false;
                }
            } else if (line == "Shader") {
                if (!loadShaderFromResourceFile(in)) {
                    LOG("could not parse and load shader");
                    return false;
                }
            }
        }

        in.close();
        return true;
    }

    void addModel(const std::string& name, Model* model) {
        if (models_.find(name) != models_.end())
            LOG_WARN("Overriding model with name: ", name);
        models_[name] = std::move(*model);
    }

    void addMaterial(const std::string& name, Material* mat) {
        if (materials_.find(name) != materials_.end())
            LOG_WARN("Overriding material with name: ", name);
        materials_[name] = std::move(*mat);
    }

    void addTexture2D(const std::string& name, Texture2D* tex) {
        if (textures2D_.find(name) != textures2D_.end())
            LOG_WARN("Overriding texture2D with name: ", name);
        textures2D_[name] = std::move(*tex);
    }

    void addShader(const std::string& name, Shader* shader) {
        if (shaders_.find(name) != shaders_.end())
            LOG_WARN("Overriding shader with name: ", name);
        shaders_[name] = std::move(*shader);
    }

} } // namespace Progression::Resource
