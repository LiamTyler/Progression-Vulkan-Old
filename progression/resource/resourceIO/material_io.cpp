#include "resource/resourceIO/material_io.hpp"
#include "resource/resourceIO/texture_io.hpp"
#include "resource/resource_manager.hpp"
#include "utils/logger.hpp"
#include <fstream>
#include "core/common.hpp"

namespace Progression {

    bool loadMtlFile(
            std::vector<std::pair<std::string, Material>>& materials,
            const std::string& fname,
            const std::string& rootTexDir)
    {
        std::ifstream file(fname);
        if (!file) {
            LOG_ERR("Could not open mtl file: ", fname);
            return false;
        }

        materials.clear();
        Material* mat = nullptr;
        std::vector<Texture2D*> newTextures; // just for cleaning up on failure

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
                ss >> mat->Ns;
            } else if (first == "Ka") {
                ss >> mat->Ka;
            } else if (first == "Kd") {
                ss >> mat->Kd;
            } else if (first == "Ks") {
                ss >> mat->Ks;
            } else if (first == "Ke") {
                ss >> mat->Ke;
            } else if (first == "map_Kd") {
                std::string texName;
                ss >> texName;
                if (Resource::getTexture2D(texName)) {
                    mat->map_Kd = Resource::getTexture2D(texName);
                    continue;
                }
                TextureUsageDesc texUsage;
                mat->map_Kd = Resource::loadTexture2D(texName, rootTexDir + texName, texUsage, true);
                if (!mat->map_Kd) {
                    LOG_ERR("Unable to load material's texture: ", rootTexDir + texName);
                    // cleanup newly created textures
                    for (const auto& t : newTextures)
                        delete t;
                    return false;
                }
            }
        }

        return true;
    }

    bool loadMaterialFromResourceFile(Material& mat, std::string& name, std::istream& in)
    {
        std::string line;
        std::string s;
        std::istringstream ss;

        // material name
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "name");
        ss >> name;
        PG_ASSERT(!in.fail() && !ss.fail());

        // ambient
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "Ka");
        ss >> mat.Ka;
        PG_ASSERT(!in.fail() && !ss.fail());

        // diffuse
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "Kd");
        ss >> mat.Kd;
        PG_ASSERT(!in.fail() && !ss.fail());

        // specular
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "Ks");
        ss >> mat.Ks;
        PG_ASSERT(!in.fail() && !ss.fail());

        // emissive
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "Ke");
        ss >> mat.Ke;
        PG_ASSERT(!in.fail() && !ss.fail());

        // Ns
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "Ns");
        ss >> mat.Ns;
        PG_ASSERT(!in.fail() && !ss.fail());

        // map_Kd
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "map_Kd");
        if (!ss.eof()) {
            ss >> s;
            PG_ASSERT(!in.fail() && !ss.fail());
            if (Resource::getTexture2D(s)) {
                mat.map_Kd = Resource::getTexture2D(s);
            } else {
                LOG_ERR("Diffuse texture '", s, "' for Material '", name, "' needs to be already loaded");
                return false;
            }
        }
        PG_ASSERT(!in.fail() && !ss.fail());

        return true;
    }

} // namespace Progression
