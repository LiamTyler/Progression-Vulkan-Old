#include "resource/resource_manager.hpp"
#include "resource/mesh.hpp"
#include "core/configuration.hpp"
#include "resource/resourceIO/io.hpp"
#include "utils/logger.hpp"
#include <sys/stat.h>
#include <thread>

namespace Progression { namespace ResourceManager {

    std::vector<std::unordered_map<std::string, Resource*>> resources;

    void init() {
        resources.resize(4);
        // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        // materials_["default"] = Material();
        // backgroundLoader_ = std::thread(loadBackgroundResources);
    }

    void update() {
    }


    void shutdown() {
    }

    // Model* loadModel(const std::string& name, const std::string& fname, bool optimize, bool freeCPUCopy) {
    //     return nullptr;
    // }

    // std::vector<Material*> loadMaterials(const std::string& fname) {
    //     return {};
    // }

    Texture2D* loadTexture2D(const std::string& name, const TextureMetaData& metaData) {
        auto resource = get<Texture2D>(name);
        LOG("got resource");
        if (resource) {
            LOG_WARN("Reloading texture that is already loaded");
        }

        Texture2D tex;
        tex.name = name;
        tex.metaData = metaData;
        if (!tex.load()) {
            return nullptr;
        }

        auto& group = resources[getResourceTypeID<Texture2D>()];
        if (resource)
            *group[name] = std::move(tex);
        else
            group[name] = new Texture2D(std::move(tex));

        return (Texture2D*) group[name];
    }

    Shader* loadShader(const std::string& name, const ShaderMetaData& metaData) {
        auto resource = get<Shader>(name);
        if (resource) {
            LOG_WARN("Reloading texture that is already loaded");
        }

        Shader shader(name, metaData);
        if (!shader.loadFromText()) {
            LOG_ERR("Failed to load shader: ", name);
            return nullptr;
        }

        auto& group = resources[getResourceTypeID<Shader>()];
        if (resource)
            *group[name] = std::move(shader);
        else
            group[name] = new Shader(std::move(shader));

        return (Shader*) group[name];
    }


/*
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
    */

    // bool loadResourceFile(const std::string& fname) {
    //     static int load = 0;
    //     load++;
    //     std::ifstream in(fname);
    //     if (!in) {
    //         LOG_ERR("Could not open resource file:", fname);
    //         return false;
    //     }

    //     std::string line;
    //     while (std::getline(in, line)) {
    //         if (line == "")
    //             continue;
    //         if (line[0] == '#')
    //             continue;
    //         if (line == "Material") {
    //             if (load > 1)
    //                 continue;
    //             if (!loadMaterialFromResourceFile(in)) {
    //                 LOG_ERR("could not parse and create material");
    //                 return false;
    //             }
    //         } else if (line == "Model") {
    //             if (load > 1)
    //                 continue;
    //             if (!loadModelFromResourceFile(in)) {
    //                 LOG_ERR("could not parse and load model");
    //                 return false;
    //             }
    //         } else if (line == "Texture2D") {
    //             if (load > 1)
    //                 continue;
    //             if (!loadTextureFromResourceFile(in)) {
    //                 LOG_ERR("Could not parse and load texture");
    //                 return false;
    //             }
    //         } else if (line == "Shader") {
    //             if (!loadShaderFromResourceFile(in)) {
    //                 LOG("could not parse and load shader");
    //                 return false;
    //             }
    //         }
    //     }

    //     in.close();

    //     resourceFiles_[fname] = TimeStamp(fname);
    //     return true;
    // }


} } // namespace Progression::Resource

namespace Progression { namespace {

/*
    bool loadShaderFromResourceFile(std::istream& in) {
        std::string line;
        std::string s;
        std::istringstream ss;
        std::string name;
        ShaderFileDesc desc;

        // shader name
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "name");
        ss >> name;
        PG_ASSERT(!in.fail() && !ss.fail());

        // vertex
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "vertex");
        if (!ss.eof())
            ss >> desc.vertex;
        PG_ASSERT(!in.fail() && !ss.fail());

        // geometry
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "geometry");
        if (!ss.eof())
            ss >> desc.geometry;
        PG_ASSERT(!in.fail() && !ss.fail());

        // vertex
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "fragment");
        if (!ss.eof())
            ss >> desc.fragment;
        PG_ASSERT(!in.fail() && !ss.fail());

        // compute
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "compute");
        if (!ss.eof())
            ss >> desc.compute;
        PG_ASSERT(!in.fail() && !ss.fail());

        addShaderRootDir(desc, PG_RESOURCE_DIR);

        ShaderTimeStamp newStamp = ShaderTimeStamp(desc);
        if (Resource::getShader(name)) {
            auto& currentStamp = shaderTimeStamps_[name];
            if (desc != currentStamp.desc || currentStamp.outOfDate(newStamp)) {
                LOG("Need to update shader: ", name);
                currentStamp = newStamp;

                Shader shader;
                if (!loadShaderFromText(shader, desc)) {
                    LOG("Could not load shader ", name);
                    return false;
                }
                updatedShaders_[name] = std::move(shader);
            } {
                LOG("already up to date shader: ", name);
            }
        } else {
            shaderTimeStamps_[name] = newStamp;
            if (!loadShaderFromText(shaders_[name], desc)) {
                LOG("Could not load shader ", name);
                shaders_.erase(name);
                return false;
            }
        }

        return true;
    }
    */
} }


    // static std::unordered_map<std::string, GLint> internalFormatMap = {
    //     { "R8",      GL_R8 },
    //     { "RG8",     GL_RG},
    //     { "RGB8",    GL_RGB },
    //     { "RGBA8",   GL_RGBA },
    //     { "R16F",    GL_R16F },
    //     { "RG16F",   GL_RG16F },
    //     { "RGB16F",  GL_RGB16F },
    //     { "RGBA16F", GL_RGBA16F },
    //     { "R32F",    GL_R32F },
    //     { "RG32F",   GL_RG32F },
    //     { "RGB32F",  GL_RGB32F },
    //     { "RGBA32F", GL_RGBA32F },
    //     { "SRGB8",   GL_SRGB8 },
    //     { "SRGBA8",  GL_SRGB8_ALPHA8 },
    //     { "DEPTH",   GL_DEPTH_COMPONENT },
    // };

    // static std::unordered_map<std::string, GLint> minFilterMap = {
    //     { "nearest", GL_NEAREST },
    //     { "linear", GL_LINEAR },
    //     { "nearest_mipmap_nearest", GL_NEAREST_MIPMAP_NEAREST },
    //     { "linear_mipmap_nearest", GL_LINEAR_MIPMAP_NEAREST },
    //     { "nearest_mipmap_linear", GL_NEAREST_MIPMAP_LINEAR },
    //     { "linear_mipmap_linear", GL_LINEAR_MIPMAP_LINEAR },
    // };

    // static std::unordered_map<std::string, GLint> magFilterMap = {
    //     { "nearest", GL_NEAREST },
    //     { "linear", GL_LINEAR },
    // };

    // static std::unordered_map<std::string, GLint> wrapMap = {
    //     { "clamp_to_edge", GL_CLAMP_TO_EDGE },
    //     { "clamp_to_border", GL_CLAMP_TO_BORDER },
    //     { "mirror_repeat", GL_MIRRORED_REPEAT },
    //     { "repeat", GL_REPEAT },
    //     { "mirror_clamp_to_edge", GL_MIRROR_CLAMP_TO_EDGE },
    // };

    // bool loadTextureFromResourceFile(std::istream& in) {
    //     std::string line;
    //     std::string s;
    //     std::istringstream ss;
    //     std::unordered_map<std::string, GLint>::iterator it;
    //     std::string fname;
    //     TextureUsageDesc desc;
    //     bool freeCPUCopy;

    //     // texture name
    //     std::getline(in, line);
    //     ss = std::istringstream(line);
    //     ss >> s;
    //     PG_ASSERT(s == "filename");
    //     ss >> fname;
    //     PG_ASSERT(!in.fail() && !ss.fail());

    //     // freeCPUCopy
    //     std::getline(in, line);
    //     ss = std::istringstream(line);
    //     ss >> s;
    //     PG_ASSERT(s == "freeCPUCopy");
    //     ss >> s;
    //     PG_ASSERT(s == "true" || s == "false");
    //     freeCPUCopy = s == "true";
    //     PG_ASSERT(!in.fail() && !ss.fail());

    //     // mipmaped
    //     std::getline(in, line);
    //     ss = std::istringstream(line);
    //     ss >> s;
    //     PG_ASSERT(s == "mipmapped");
    //     ss >> s;
    //     PG_ASSERT(s == "true" || s == "false");
    //     desc.mipMapped = s == "true";
    //     PG_ASSERT(!in.fail() && !ss.fail());

    //     // internal format
    //     std::getline(in, line);
    //     ss = std::istringstream(line);
    //     ss >> s;
    //     PG_ASSERT(s == "internalFormat");
    //     ss >> s;
    //     it = internalFormatMap.find(s);
    //     if (it == internalFormatMap.end()) {
    //         LOG_ERR("Invalid texture format: ", s);
    //         return false;
    //     }
    //     desc.internalFormat = it->second;
    //     PG_ASSERT(!in.fail() && !ss.fail());


    //     // min filter
    //     std::getline(in, line);
    //     ss = std::istringstream(line);
    //     ss >> s;
    //     PG_ASSERT(s == "minFilter");
    //     ss >> s;
    //     it = minFilterMap.find(s);
    //     if (it == minFilterMap.end()) {
    //         LOG_ERR("Invalid minFilter format: ", s);
    //         return false;
    //     }
    //     if (!desc.mipMapped && (s != "nearest" && s != "linear")) {
    //         LOG_WARN("Trying to use a mip map min filter when there is no mip map on texture: ", fname);
    //     }
    //     desc.minFilter = it->second;
    //     PG_ASSERT(!in.fail() && !ss.fail());

    //     // mag filter
    //     std::getline(in, line);
    //     ss = std::istringstream(line);
    //     ss >> s;
    //     PG_ASSERT(s == "magFilter");
    //     ss >> s;
    //     it = magFilterMap.find(s);
    //     if (it == magFilterMap.end()) {
    //         LOG_ERR("Invalid magFilter format: ", s);
    //         return false;
    //     }
    //     desc.magFilter = it->second;
    //     PG_ASSERT(!in.fail() && !ss.fail());

    //     // wrapModeS
    //     std::getline(in, line);
    //     ss = std::istringstream(line);
    //     ss >> s;
    //     PG_ASSERT(s == "wrapModeS");
    //     ss >> s;
    //     it = wrapMap.find(s);
    //     if (it == wrapMap.end()) {
    //         LOG_ERR("Invalid wrapModeS format: ", s);
    //         return false;
    //     }
    //     desc.wrapModeS = it->second;
    //     PG_ASSERT(!in.fail() && !ss.fail());

    //     // wrapModeT
    //     std::getline(in, line);
    //     ss = std::istringstream(line);
    //     ss >> s;
    //     PG_ASSERT(s == "wrapModeT");
    //     ss >> s;
    //     it = wrapMap.find(s);
    //     if (it == wrapMap.end()) {
    //         LOG_ERR("Invalid wrapModeT format: ", s);
    //         return false;
    //     }
    //     desc.wrapModeT = it->second;
    //     PG_ASSERT(!in.fail() && !ss.fail());

    //     return Resource::loadTexture2D(fname, PG_RESOURCE_DIR + fname, desc, freeCPUCopy) != nullptr;
    // }