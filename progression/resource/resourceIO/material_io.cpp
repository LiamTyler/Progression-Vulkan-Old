#include "resource/resourceIO/material_io.hpp"
#include "resource/resourceIO/texture_io.hpp"
#include "utils/logger.hpp"
#include <fstream>
#include "core/common.hpp"

namespace Progression {

    bool loadMtlFile(
            std::vector<std::pair<std::string, Material>>& materials,
            const std::string& fname,
            const std::string& rootTexDir,
            std::unordered_map<std::string, Texture2D>& existingTextureList)
    {
        std::ifstream file(fname);
        if (!file) {
            LOG_ERR("Could not open mtl file: ", fname);
            return false;
        }

        materials.clear();
        Material* mat = nullptr;
        std::unordered_map<std::string, Texture2D*> newTextures; // just for cleaning up on failure

        std::string line;
        std::string first;
        while (std::getline(file, line)) {
            std::istringstream ss(line);
            ss >> first;
            if (first == "#") {
                continue;
            } else if (first == "newmtl") {
                std::string name;
                ss >> name;
                materials.emplace_back(name, Material{});
                mat = &materials[materials.size() - 1].second;
            } else if (first == "Ns") {
                ss >> mat->shininess;
            } else if (first == "Ka") {
                ss >> mat->ambient;
            } else if (first == "Kd") {
                ss >> mat->diffuse;
            } else if (first == "Ks") {
                ss >> mat->specular;
            } else if (first == "Ke") {
                ss >> mat->emissive;
            } else if (first == "map_Kd") {
                std::string texName;
                ss >> texName;
                auto it = existingTextureList.find(texName);
                if (it != existingTextureList.end()) {
                    mat->diffuseTexture = &(it->second);
                } else {
                    Texture2D* tex = new Texture2D;
                    TextureUsageDesc desc;
                    if (!loadTexture2D(*tex, rootTexDir + texName, desc, true)) {
                        LOG("Could not load the diffuse texture '", rootTexDir + texName, "' for "
                            "material '", materials[materials.size() - 1].first, "' in mtl file: ", \
                            fname);
                        // cleanup newly created textures
                        for (const auto& pair: newTextures)
                            delete pair.second;

                        return false;
                    } else {
                        newTextures[texName] = tex;
                        mat->diffuseTexture = tex;
                    }
                }
            }
        }

        return true;
    }

} // namespace Progression
