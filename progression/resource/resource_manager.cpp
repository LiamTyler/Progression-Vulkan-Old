#include "resource/resource_manager.hpp"
#include "resource/mesh.hpp"
#include "core/configuration.hpp"
#include "resource/resourceIO/io.hpp"
#include "utils/logger.hpp"

namespace Progression { namespace Resource {

        void init();
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


        Model* loadModel(const std::string& fname, bool optimize, bool freeCPUCopy) {
            if (getModel(fname)) {
                LOG_WARN("Reloading model that is already loaded");
            }

            models_[fname] = std::move(Model());
            if (!loadModelFromObj(models_[fname], fname, optimize, freeCPUCopy)) {
                LOG("Could not load model file: ", fname);
                models_.erase(fname);
                return nullptr;
            }
            return &models_[fname];
        }

        std::vector<Material*> loadMaterials(const std::string& fname) {
            std::vector<std::pair<std::string, Material>> materials;
            if (!loadMtlFile(materials, fname, PG_RESOURCE_DIR, textures2D_)) {
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

        Texture2D* loadTexture2D(const std::string& fname, const TextureUsageDesc& desc) {
            if (getTexture2D(fname)) {
                LOG_WARN("Reloading texture that is already loaded");
            }

            if (!loadTexture2D(textures2D_[fname], fname, desc)) {
                LOG("Could not load texture file: ", fname);
                textures2D_.erase(fname);
                return nullptr;
            }

            return &textures2D_[fname];
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

} } // namespace Progression::Resource
