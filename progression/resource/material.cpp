#include "resource/material.hpp"
#include "resource/resource_manager.hpp"
#include "resource/texture2D.hpp"

namespace Progression {

    bool Material::load(MetaData* metaData) {
        UNUSED(metaData);
        return false;
    }

    void Material::move(Resource* resource) {
        Material& newResource = *(Material*) resource;
        newResource = std::move(*this);
    }

    bool Material::readMetaData(std::istream& in) {
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
        ss >> Ka;
        PG_ASSERT(!in.fail() && !ss.fail());

        // diffuse
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "Kd");
        ss >> Kd;
        PG_ASSERT(!in.fail() && !ss.fail());

        // specular
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "Ks");
        ss >> Ks;
        PG_ASSERT(!in.fail() && !ss.fail());

        // emissive
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "Ke");
        ss >> Ke;
        PG_ASSERT(!in.fail() && !ss.fail());

        // Ns
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "Ns");
        ss >> Ns;
        PG_ASSERT(!in.fail() && !ss.fail());

        // map_Kd
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "map_Kd");
        if (!ss.eof()) {
            ss >> s;
            PG_ASSERT(!in.fail() && !ss.fail());
            map_Kd_name = s;
        }
        PG_ASSERT(!in.fail() && !ss.fail());

        return !in.fail() && !ss.fail();
    }

    ResUpdateStatus Material::loadFromResourceFile(std::istream& in, std::function<void()>& updateFunc)
    {
        UNUSED(updateFunc);
        return readMetaData(in) ? RES_RELOAD_SUCCESS : RES_PARSE_ERROR;
    }

    bool Material::loadMtlFile(
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
                if (ResourceManager::get<Texture2D>(texName)) {
                    mat->map_Kd = ResourceManager::get<Texture2D>(texName).get();
                    continue;
                }
                TextureMetaData meta;
                meta.file = TimeStampedFile(rootTexDir + texName);
                mat->map_Kd = ResourceManager::load<Texture2D>(texName, &meta).get();
                if (!mat->map_Kd) {
                    LOG_ERR("Unable to load material's texture: ", rootTexDir + texName);
                    // cleanup newly created textures
                    for (const auto& t : newTextures)
                        delete t;
                    return false;
                } else {
                    newTextures.push_back(mat->map_Kd);
                }
            }
        }

        return true;
    }

} // namespace Progression
